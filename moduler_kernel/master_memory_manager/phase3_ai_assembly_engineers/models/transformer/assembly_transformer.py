
#!/usr/bin/env python3
"""
Assembly Transformer - Transformer-based model for assembly code generation
Part of Phase 3: AI Assembly Engineers for BDI
"""

import torch
import torch.nn as nn
import torch.nn.functional as F
import numpy as np
import json
import time
import logging
from typing import Dict, List, Optional, Any, Tuple
from dataclasses import dataclass
import math
import os

@dataclass
class TransformerConfig:
    vocab_size: int = 10000
    max_seq_length: int = 512
    d_model: int = 512
    num_heads: int = 8
    num_layers: int = 6
    d_ff: int = 2048
    dropout: float = 0.1
    activation: str = "relu"
    layer_norm_eps: float = 1e-5
    pad_token_id: int = 0
    bos_token_id: int = 1
    eos_token_id: int = 2

class PositionalEncoding(nn.Module):
    """Positional encoding for transformer"""
    
    def __init__(self, d_model: int, max_len: int = 5000):
        super().__init__()
        
        pe = torch.zeros(max_len, d_model)
        position = torch.arange(0, max_len, dtype=torch.float).unsqueeze(1)
        
        div_term = torch.exp(torch.arange(0, d_model, 2).float() * 
                           (-math.log(10000.0) / d_model))
        
        pe[:, 0::2] = torch.sin(position * div_term)
        pe[:, 1::2] = torch.cos(position * div_term)
        pe = pe.unsqueeze(0).transpose(0, 1)
        
        self.register_buffer('pe', pe)
    
    def forward(self, x):
        return x + self.pe[:x.size(0), :]

class MultiHeadAttention(nn.Module):
    """Multi-head attention mechanism"""
    
    def __init__(self, d_model: int, num_heads: int, dropout: float = 0.1):
        super().__init__()
        assert d_model % num_heads == 0
        
        self.d_model = d_model
        self.num_heads = num_heads
        self.d_k = d_model // num_heads
        
        self.w_q = nn.Linear(d_model, d_model)
        self.w_k = nn.Linear(d_model, d_model)
        self.w_v = nn.Linear(d_model, d_model)
        self.w_o = nn.Linear(d_model, d_model)
        
        self.dropout = nn.Dropout(dropout)
        
    def forward(self, query, key, value, mask=None):
        batch_size = query.size(0)
        
        # Linear transformations and split into heads
        Q = self.w_q(query).view(batch_size, -1, self.num_heads, self.d_k).transpose(1, 2)
        K = self.w_k(key).view(batch_size, -1, self.num_heads, self.d_k).transpose(1, 2)
        V = self.w_v(value).view(batch_size, -1, self.num_heads, self.d_k).transpose(1, 2)
        
        # Attention
        attention_output, attention_weights = self.attention(Q, K, V, mask)
        
        # Concatenate heads
        attention_output = attention_output.transpose(1, 2).contiguous().view(
            batch_size, -1, self.d_model)
        
        # Final linear transformation
        output = self.w_o(attention_output)
        
        return output, attention_weights
    
    def attention(self, Q, K, V, mask=None):
        d_k = Q.size(-1)
        scores = torch.matmul(Q, K.transpose(-2, -1)) / math.sqrt(d_k)
        
        if mask is not None:
            scores = scores.masked_fill(mask == 0, -1e9)
        
        attention_weights = F.softmax(scores, dim=-1)
        attention_weights = self.dropout(attention_weights)
        
        output = torch.matmul(attention_weights, V)
        
        return output, attention_weights

class FeedForward(nn.Module):
    """Feed-forward network"""
    
    def __init__(self, d_model: int, d_ff: int, dropout: float = 0.1, activation: str = "relu"):
        super().__init__()
        
        self.linear1 = nn.Linear(d_model, d_ff)
        self.linear2 = nn.Linear(d_ff, d_model)
        self.dropout = nn.Dropout(dropout)
        
        if activation == "relu":
            self.activation = F.relu
        elif activation == "gelu":
            self.activation = F.gelu
        else:
            raise ValueError(f"Unsupported activation: {activation}")
    
    def forward(self, x):
        return self.linear2(self.dropout(self.activation(self.linear1(x))))

class TransformerBlock(nn.Module):
    """Transformer encoder/decoder block"""
    
    def __init__(self, config: TransformerConfig, is_decoder: bool = False):
        super().__init__()
        
        self.is_decoder = is_decoder
        self.self_attention = MultiHeadAttention(config.d_model, config.num_heads, config.dropout)
        
        if is_decoder:
            self.cross_attention = MultiHeadAttention(config.d_model, config.num_heads, config.dropout)
            self.norm2 = nn.LayerNorm(config.d_model, eps=config.layer_norm_eps)
        
        self.feed_forward = FeedForward(config.d_model, config.d_ff, config.dropout, config.activation)
        
        self.norm1 = nn.LayerNorm(config.d_model, eps=config.layer_norm_eps)
        self.norm3 = nn.LayerNorm(config.d_model, eps=config.layer_norm_eps)
        
        self.dropout = nn.Dropout(config.dropout)
    
    def forward(self, x, encoder_output=None, self_attention_mask=None, cross_attention_mask=None):
        # Self-attention
        attn_output, _ = self.self_attention(x, x, x, self_attention_mask)
        x = self.norm1(x + self.dropout(attn_output))
        
        # Cross-attention (decoder only)
        if self.is_decoder and encoder_output is not None:
            cross_attn_output, _ = self.cross_attention(x, encoder_output, encoder_output, cross_attention_mask)
            x = self.norm2(x + self.dropout(cross_attn_output))
        
        # Feed-forward
        ff_output = self.feed_forward(x)
        x = self.norm3(x + self.dropout(ff_output))
        
        return x

class AssemblyTransformer(nn.Module):
    """Transformer model for assembly code generation"""
    
    def __init__(self, config: TransformerConfig):
        super().__init__()
        
        self.config = config
        
        # Embeddings
        self.token_embedding = nn.Embedding(config.vocab_size, config.d_model)
        self.position_encoding = PositionalEncoding(config.d_model, config.max_seq_length)
        
        # Encoder
        self.encoder_layers = nn.ModuleList([
            TransformerBlock(config, is_decoder=False) 
            for _ in range(config.num_layers)
        ])
        
        # Decoder
        self.decoder_layers = nn.ModuleList([
            TransformerBlock(config, is_decoder=True) 
            for _ in range(config.num_layers)
        ])
        
        # Output projection
        self.output_projection = nn.Linear(config.d_model, config.vocab_size)
        
        # Initialize weights
        self.apply(self._init_weights)
    
    def _init_weights(self, module):
        if isinstance(module, nn.Linear):
            torch.nn.init.normal_(module.weight, mean=0.0, std=0.02)
            if module.bias is not None:
                torch.nn.init.zeros_(module.bias)
        elif isinstance(module, nn.Embedding):
            torch.nn.init.normal_(module.weight, mean=0.0, std=0.02)
        elif isinstance(module, nn.LayerNorm):
            torch.nn.init.zeros_(module.bias)
            torch.nn.init.ones_(module.weight)
    
    def forward(self, input_ids, target_ids=None, attention_mask=None):
        # Encode input
        encoder_output = self.encode(input_ids, attention_mask)
        
        if target_ids is not None:
            # Training mode - decode with teacher forcing
            decoder_output = self.decode(target_ids, encoder_output, attention_mask)
            logits = self.output_projection(decoder_output)
            return logits
        else:
            # Inference mode - generate sequence
            return self.generate(encoder_output, attention_mask)
    
    def encode(self, input_ids, attention_mask=None):
        # Token embeddings
        embeddings = self.token_embedding(input_ids)
        embeddings = self.position_encoding(embeddings.transpose(0, 1)).transpose(0, 1)
        
        # Encoder layers
        hidden_states = embeddings
        for layer in self.encoder_layers:
            hidden_states = layer(hidden_states, self_attention_mask=attention_mask)
        
        return hidden_states
    
    def decode(self, target_ids, encoder_output, encoder_attention_mask=None):
        # Target embeddings
        embeddings = self.token_embedding(target_ids)
        embeddings = self.position_encoding(embeddings.transpose(0, 1)).transpose(0, 1)
        
        # Create causal mask for decoder
        seq_len = target_ids.size(1)
        causal_mask = torch.tril(torch.ones(seq_len, seq_len)).unsqueeze(0).unsqueeze(0)
        causal_mask = causal_mask.to(target_ids.device)
        
        # Decoder layers
        hidden_states = embeddings
        for layer in self.decoder_layers:
            hidden_states = layer(
                hidden_states, 
                encoder_output=encoder_output,
                self_attention_mask=causal_mask,
                cross_attention_mask=encoder_attention_mask
            )
        
        return hidden_states
    
    def generate(self, encoder_output, encoder_attention_mask=None, max_length=None, temperature=1.0):
        """Generate assembly code sequence"""
        if max_length is None:
            max_length = self.config.max_seq_length
        
        batch_size = encoder_output.size(0)
        device = encoder_output.device
        
        # Start with BOS token
        generated = torch.full((batch_size, 1), self.config.bos_token_id, dtype=torch.long, device=device)
        
        for _ in range(max_length - 1):
            # Decode current sequence
            decoder_output = self.decode(generated, encoder_output, encoder_attention_mask)
            
            # Get logits for next token
            logits = self.output_projection(decoder_output[:, -1, :])
            
            # Apply temperature
            if temperature != 1.0:
                logits = logits / temperature
            
            # Sample next token
            probs = F.softmax(logits, dim=-1)
            next_token = torch.multinomial(probs, 1)
            
            # Append to sequence
            generated = torch.cat([generated, next_token], dim=1)
            
            # Check for EOS token
            if (next_token == self.config.eos_token_id).all():
                break
        
        return generated

class AssemblyTokenizer:
    """Tokenizer for assembly code"""
    
    def __init__(self, vocab_file: Optional[str] = None):
        self.vocab = {}
        self.inverse_vocab = {}
        self.special_tokens = {
            '<PAD>': 0,
            '<BOS>': 1,
            '<EOS>': 2,
            '<UNK>': 3
        }
        
        if vocab_file and os.path.exists(vocab_file):
            self.load_vocab(vocab_file)
        else:
            self._build_default_vocab()
    
    def _build_default_vocab(self):
        """Build default vocabulary for x86 assembly"""
        
        # Start with special tokens
        self.vocab.update(self.special_tokens)
        
        # x86 instructions
        instructions = [
            'mov', 'add', 'sub', 'mul', 'div', 'imul', 'idiv',
            'and', 'or', 'xor', 'not', 'shl', 'shr', 'sal', 'sar',
            'push', 'pop', 'pushad', 'popad',
            'jmp', 'je', 'jne', 'jg', 'jl', 'jge', 'jle', 'ja', 'jb',
            'call', 'ret', 'iret', 'int', 'loop',
            'cmp', 'test', 'inc', 'dec', 'neg',
            'lea', 'movzx', 'movsx', 'xchg',
            'nop', 'hlt', 'cli', 'sti'
        ]
        
        # Registers
        registers = [
            'eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'esp', 'ebp',
            'ax', 'bx', 'cx', 'dx', 'si', 'di', 'sp', 'bp',
            'al', 'bl', 'cl', 'dl', 'ah', 'bh', 'ch', 'dh',
            'cs', 'ds', 'es', 'fs', 'gs', 'ss',
            'cr0', 'cr1', 'cr2', 'cr3', 'cr4',
            'dr0', 'dr1', 'dr2', 'dr3', 'dr6', 'dr7'
        ]
        
        # Directives and keywords
        directives = [
            'section', 'global', 'extern', 'db', 'dw', 'dd', 'dq',
            'times', 'align', 'bits', 'org', 'resb', 'resw', 'resd'
        ]
        
        # Common symbols
        symbols = [',', '[', ']', '+', '-', '*', ':', ';', '(', ')']
        
        # Numbers (placeholders)
        numbers = [f'<NUM_{i}>' for i in range(100)]
        
        # Labels (placeholders)
        labels = [f'<LABEL_{i}>' for i in range(50)]
        
        # Build vocabulary
        current_id = len(self.special_tokens)
        
        for token_list in [instructions, registers, directives, symbols, numbers, labels]:
            for token in token_list:
                if token not in self.vocab:
                    self.vocab[token] = current_id
                    current_id += 1
        
        # Build inverse vocabulary
        self.inverse_vocab = {v: k for k, v in self.vocab.items()}
    
    def tokenize(self, text: str) -> List[str]:
        """Tokenize assembly code text"""
        # Simple tokenization - split by whitespace and common delimiters
        import re
        
        # Replace numbers with placeholders
        text = re.sub(r'\b\d+\b', lambda m: f'<NUM_{min(int(m.group()) % 100, 99)}>', text)
        
        # Replace hex numbers
        text = re.sub(r'\b0x[0-9a-fA-F]+\b', lambda m: f'<NUM_{hash(m.group()) % 100}>', text)
        
        # Split on whitespace and punctuation
        tokens = re.findall(r'\w+|[^\w\s]', text.lower())
        
        return tokens
    
    def encode(self, text: str) -> List[int]:
        """Encode text to token IDs"""
        tokens = self.tokenize(text)
        token_ids = []
        
        for token in tokens:
            if token in self.vocab:
                token_ids.append(self.vocab[token])
            else:
                token_ids.append(self.vocab['<UNK>'])
        
        return token_ids
    
    def decode(self, token_ids: List[int]) -> str:
        """Decode token IDs to text"""
        tokens = []
        
        for token_id in token_ids:
            if token_id in self.inverse_vocab:
                token = self.inverse_vocab[token_id]
                if not token.startswith('<') or token in ['<BOS>', '<EOS>']:
                    tokens.append(token)
        
        return ' '.join(tokens)
    
    def save_vocab(self, vocab_file: str):
        """Save vocabulary to file"""
        with open(vocab_file, 'w') as f:
            json.dump(self.vocab, f, indent=2)
    
    def load_vocab(self, vocab_file: str):
        """Load vocabulary from file"""
        with open(vocab_file, 'r') as f:
            self.vocab = json.load(f)
        
        self.inverse_vocab = {v: k for k, v in self.vocab.items()}

class AssemblyTrainer:
    """Training system for assembly transformer"""
    
    def __init__(self, model: AssemblyTransformer, tokenizer: AssemblyTokenizer, config: Dict[str, Any]):
        self.model = model
        self.tokenizer = tokenizer
        self.config = config
        
        # Setup optimizer
        self.optimizer = torch.optim.AdamW(
            model.parameters(),
            lr=config.get('learning_rate', 1e-4),
            weight_decay=config.get('weight_decay', 0.01)
        )
        
        # Setup scheduler
        self.scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(
            self.optimizer,
            T_max=config.get('max_epochs', 100)
        )
        
        # Setup logging
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
        
        # Training state
        self.current_epoch = 0
        self.global_step = 0
        self.best_loss = float('inf')
    
    def train_step(self, batch):
        """Single training step"""
        self.model.train()
        
        input_ids = batch['input_ids']
        target_ids = batch['target_ids']
        attention_mask = batch.get('attention_mask')
        
        # Forward pass
        logits = self.model(input_ids, target_ids, attention_mask)
        
        # Calculate loss
        loss = F.cross_entropy(
            logits.view(-1, logits.size(-1)),
            target_ids.view(-1),
            ignore_index=self.tokenizer.vocab['<PAD>']
        )
        
        # Backward pass
        self.optimizer.zero_grad()
        loss.backward()
        
        # Gradient clipping
        torch.nn.utils.clip_grad_norm_(self.model.parameters(), max_norm=1.0)
        
        self.optimizer.step()
        self.global_step += 1
        
        return loss.item()
    
    def validate(self, val_dataloader):
        """Validation step"""
        self.model.eval()
        total_loss = 0
        num_batches = 0
        
        with torch.no_grad():
            for batch in val_dataloader:
                input_ids = batch['input_ids']
                target_ids = batch['target_ids']
                attention_mask = batch.get('attention_mask')
                
                logits = self.model(input_ids, target_ids, attention_mask)
                
                loss = F.cross_entropy(
                    logits.view(-1, logits.size(-1)),
                    target_ids.view(-1),
                    ignore_index=self.tokenizer.vocab['<PAD>']
                )
                
                total_loss += loss.item()
                num_batches += 1
        
        return total_loss / num_batches if num_batches > 0 else 0
    
    def train(self, train_dataloader, val_dataloader=None, num_epochs=None):
        """Full training loop"""
        if num_epochs is None:
            num_epochs = self.config.get('max_epochs', 100)
        
        self.logger.info(f"Starting training for {num_epochs} epochs")
        
        for epoch in range(num_epochs):
            self.current_epoch = epoch
            epoch_loss = 0
            num_batches = 0
            
            # Training
            for batch in train_dataloader:
                loss = self.train_step(batch)
                epoch_loss += loss
                num_batches += 1
                
                if self.global_step % 100 == 0:
                    self.logger.info(f"Step {self.global_step}, Loss: {loss:.4f}")
            
            avg_train_loss = epoch_loss / num_batches if num_batches > 0 else 0
            
            # Validation
            val_loss = 0
            if val_dataloader:
                val_loss = self.validate(val_dataloader)
            
            # Update scheduler
            self.scheduler.step()
            
            # Log epoch results
            self.logger.info(
                f"Epoch {epoch + 1}/{num_epochs}, "
                f"Train Loss: {avg_train_loss:.4f}, "
                f"Val Loss: {val_loss:.4f}, "
                f"LR: {self.scheduler.get_last_lr()[0]:.6f}"
            )
            
            # Save best model
            if val_loss < self.best_loss:
                self.best_loss = val_loss
                self.save_checkpoint('best_model.pt')
        
        self.logger.info("Training completed")
    
    def save_checkpoint(self, filepath: str):
        """Save model checkpoint"""
        checkpoint = {
            'model_state_dict': self.model.state_dict(),
            'optimizer_state_dict': self.optimizer.state_dict(),
            'scheduler_state_dict': self.scheduler.state_dict(),
            'current_epoch': self.current_epoch,
            'global_step': self.global_step,
            'best_loss': self.best_loss,
            'config': self.model.config.__dict__
        }
        
        torch.save(checkpoint, filepath)
        self.logger.info(f"Checkpoint saved to {filepath}")
    
    def load_checkpoint(self, filepath: str):
        """Load model checkpoint"""
        checkpoint = torch.load(filepath, map_location='cpu')
        
        self.model.load_state_dict(checkpoint['model_state_dict'])
        self.optimizer.load_state_dict(checkpoint['optimizer_state_dict'])
        self.scheduler.load_state_dict(checkpoint['scheduler_state_dict'])
        
        self.current_epoch = checkpoint['current_epoch']
        self.global_step = checkpoint['global_step']
        self.best_loss = checkpoint['best_loss']
        
        self.logger.info(f"Checkpoint loaded from {filepath}")

def create_sample_dataset():
    """Create sample dataset for demonstration"""
    samples = [
        {
            'input': 'move value from register A to register B',
            'output': 'mov ebx, eax'
        },
        {
            'input': 'add two numbers and store result',
            'output': 'add eax, ebx'
        },
        {
            'input': 'call system exit function',
            'output': 'mov eax, 1\nmov ebx, 0\nint 0x80'
        },
        {
            'input': 'save all registers on stack',
            'output': 'pushad'
        },
        {
            'input': 'restore all registers from stack',
            'output': 'popad'
        }
    ]
    
    return samples

def main():
    """Demonstrate assembly transformer"""
    
    # Configuration
    config = TransformerConfig(
        vocab_size=1000,
        max_seq_length=128,
        d_model=256,
        num_heads=8,
        num_layers=4,
        d_ff=1024
    )
    
    # Initialize components
    tokenizer = AssemblyTokenizer()
    model = AssemblyTransformer(config)
    
    print(f"Model initialized with {sum(p.numel() for p in model.parameters())} parameters")
    
    # Create sample data
    samples = create_sample_dataset()
    
    # Tokenize samples
    for sample in samples[:2]:
        input_text = sample['input']
        output_text = sample['output']
        
        input_tokens = tokenizer.encode(input_text)
        output_tokens = tokenizer.encode(output_text)
        
        print(f"\nInput: {input_text}")
        print(f"Input tokens: {input_tokens}")
        print(f"Output: {output_text}")
        print(f"Output tokens: {output_tokens}")
        
        # Decode back
        decoded_input = tokenizer.decode(input_tokens)
        decoded_output = tokenizer.decode(output_tokens)
        
        print(f"Decoded input: {decoded_input}")
        print(f"Decoded output: {decoded_output}")
    
    # Test model forward pass
    batch_size = 2
    seq_len = 10
    
    input_ids = torch.randint(0, config.vocab_size, (batch_size, seq_len))
    target_ids = torch.randint(0, config.vocab_size, (batch_size, seq_len))
    
    with torch.no_grad():
        logits = model(input_ids, target_ids)
        print(f"\nModel output shape: {logits.shape}")
        
        # Test generation
        generated = model.generate(model.encode(input_ids), max_length=20)
        print(f"Generated sequence shape: {generated.shape}")

if __name__ == "__main__":
    main()
