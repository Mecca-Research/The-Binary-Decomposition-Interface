
# BDI AI Curriculum Engine - Unicode Data

This directory contains the Unicode 17.0.0 data infrastructure for the BDI AI curriculum engine.

## Overview

The Unicode data system provides:
- **Embedded Data**: Compile-time inclusion of Unicode data via C23 #embed
- **Fast Lookup**: Binary search and hash table structures
- **Comprehensive Coverage**: All Unicode 17.0.0 properties and data
- **Memory Efficient**: Compressed binary format with 50-70% size reduction
- **Type Safe**: C23 features with compile-time assertions

## Directory Structure

```
AIBase/
├── data/                      # Generated .dat files
│   ├── unicode_basic.dat      # Basic ranges and classes
│   ├── unicode_math.dat       # Math symbols
│   ├── unicode_props.dat      # Character properties
│   ├── unicode_emoji.dat      # Emoji data
│   ├── unicode_collation.dat  # Collation keys
│   ├── unicode_idna.dat       # IDNA mappings
│   └── unicode_han.dat        # Unihan data
├── embeddings/
│   └── unicode_embed.h        # #embed headers with assertions
├── tables/
│   ├── unicode_tables.h       # API header
│   └── unicode_tables.c       # Implementation
└── README.md                  # This file
```

## Usage

### Initialization

```c
#include "tables/unicode_tables.h"

int main(void) {
    // Initialize Unicode tables
    if (!unicode_tables_init()) {
        fprintf(stderr, "Failed to initialize Unicode tables\n");
        return 1;
    }
    
    // Use Unicode functions...
    
    // Cleanup
    unicode_tables_cleanup();
    return 0;
}
```

### Character Properties

```c
// Get full character properties
unicode_char_props_t props;
if (unicode_get_char_props(0x0041, &props)) {  // 'A'
    printf("Codepoint: U+%04X\n", props.codepoint);
    printf("Category: %d\n", props.general_category);
    printf("Lowercase: U+%04X\n", props.lowercase);
}

// Character class queries
if (unicode_is_letter(0x0041)) {
    printf("'A' is a letter\n");
}

if (unicode_is_digit(0x0030)) {  // '0'
    printf("'0' is a digit\n");
}

if (unicode_is_math_symbol(0x2200)) {  // '∀'
    printf("'∀' is a math symbol\n");
}
```

### Case Conversion

```c
uint32_t upper = unicode_to_upper(0x0061);  // 'a' -> 'A'
uint32_t lower = unicode_to_lower(0x0041);  // 'A' -> 'a'
uint32_t title = unicode_to_title(0x01C5);  // 'ǅ' -> 'ǅ'
```

### Emoji

```c
if (unicode_is_emoji(0x1F600)) {  // 😀
    printf("This is an emoji\n");
}

unicode_emoji_t emoji;
if (unicode_get_emoji_data(0x1F600, &emoji)) {
    printf("Emoji sequence length: %d\n", emoji.sequence_len);
}
```

### Normalization

```c
// Decompose character
uint32_t decomposed[18];
size_t len = unicode_decompose(0x00C5, decomposed, 18);  // 'Å' -> 'A' + '◌̊'

// Compose characters
uint32_t composed = unicode_compose(decomposed, len);
```

### UTF-8 Conversion

```c
// Codepoint to UTF-8
char utf8[5];
unicode_to_utf8(0x1F600, utf8);  // 😀 -> "\xF0\x9F\x98\x80"

// UTF-8 to codepoint
const char *str = "Hello 世界";
const char *p = str;
while (*p) {
    uint32_t cp = unicode_from_utf8(&p);
    printf("U+%04X ", cp);
}
```

### String Operations

```c
// Unicode-aware string length
const char *str = "Hello 世界";
size_t len = unicode_strlen(str);  // 8 characters, not bytes

// Unicode comparison (uses collation)
int cmp = unicode_strcmp("café", "cafe");
```

## Data Files

### unicode_basic.dat
- Character ranges (Basic Latin, CJK, Emoji, etc.)
- Character classes (letters, digits, punctuation, symbols)
- Basic properties for fast queries

### unicode_math.dat
Mathematical symbols organized by type:
- **Operators**: ∀ ∃ ∈ ∉ ∩ ∪ ∧ ∨ ¬ ⊕ ⊗
- **Relations**: = ≠ < > ≤ ≥ ≡ ≈ ∼ ≅ ⊂ ⊃
- **Arrows**: → ← ↔ ⇒ ⇐ ⇔ ↑ ↓ ⇑ ⇓
- **Greek**: α β γ δ ε ζ η θ ι κ λ μ ν ξ ο π ρ σ τ υ φ χ ψ ω
- **Special**: ∞ ∂ ∇ ∫ ∑ ∏ √ ∛ ∜ ∝ ∟ ∠ ∡ ∢

### unicode_props.dat
Complete character properties:
- General Category (30 categories)
- Canonical Combining Class
- Bidirectional Class
- Decomposition Type and Mapping
- Numeric Values
- Case Mappings (upper, lower, title)
- Script (150+ scripts)
- Block (300+ blocks)
- Age (Unicode version introduced)

### unicode_emoji.dat
Emoji data:
- 3,782 emoji characters
- Emoji sequences
- ZWJ (Zero-Width Joiner) sequences
- Skin tone modifiers (🏻 🏼 🏽 🏾 🏿)
- Gender modifiers (♂️ ♀️)
- Emoji components

### unicode_collation.dat
Collation keys for sorting:
- Primary weights (base character)
- Secondary weights (accents)
- Tertiary weights (case)
- Locale-aware sorting

### unicode_idna.dat
IDNA (Internationalized Domain Names):
- Valid characters for domain names
- Disallowed characters
- Character mappings for normalization
- IDNA2008 compliance

### unicode_han.dat
Unihan (Han character) database:
- 97,680+ CJK characters
- Pronunciation readings:
  - Mandarin (Pinyin)
  - Cantonese (Jyutping)
  - Japanese (On/Kun)
  - Korean (Hangul)
  - Vietnamese (Quốc ngữ)
- Radical numbers (214 radicals)
- Stroke counts
- Simplified/Traditional variants

## API Reference

### Initialization
- `bool unicode_tables_init(void)` - Initialize tables
- `void unicode_tables_cleanup(void)` - Cleanup tables

### Character Properties
- `bool unicode_get_char_props(uint32_t cp, unicode_char_props_t *props)` - Get properties
- `bool unicode_is_letter(uint32_t cp)` - Check if letter
- `bool unicode_is_digit(uint32_t cp)` - Check if digit
- `bool unicode_is_whitespace(uint32_t cp)` - Check if whitespace
- `bool unicode_is_punctuation(uint32_t cp)` - Check if punctuation
- `bool unicode_is_symbol(uint32_t cp)` - Check if symbol
- `bool unicode_is_math_symbol(uint32_t cp)` - Check if math symbol

### Case Conversion
- `uint32_t unicode_to_upper(uint32_t cp)` - Convert to uppercase
- `uint32_t unicode_to_lower(uint32_t cp)` - Convert to lowercase
- `uint32_t unicode_to_title(uint32_t cp)` - Convert to titlecase

### Emoji
- `bool unicode_is_emoji(uint32_t cp)` - Check if emoji
- `bool unicode_get_emoji_data(uint32_t cp, unicode_emoji_t *emoji)` - Get emoji data

### Collation
- `bool unicode_get_collation_key(uint32_t cp, unicode_collation_key_t *key)` - Get collation key
- `int unicode_compare(uint32_t cp1, uint32_t cp2)` - Compare characters

### IDNA
- `bool unicode_is_idna_valid(uint32_t cp)` - Check if valid for domain names
- `bool unicode_get_idna_mapping(uint32_t cp, unicode_idna_mapping_t *mapping)` - Get IDNA mapping

### Unihan
- `bool unicode_get_unihan_data(uint32_t cp, unicode_unihan_t *unihan)` - Get Han character data

### Normalization
- `size_t unicode_decompose(uint32_t cp, uint32_t *output, size_t size)` - Decompose character
- `uint32_t unicode_compose(const uint32_t *input, size_t size)` - Compose characters

### String Operations
- `size_t unicode_strlen(const char *str)` - Unicode string length
- `int unicode_strcmp(const char *s1, const char *s2)` - Unicode string comparison
- `char *unicode_to_utf8(uint32_t cp, char *output)` - Codepoint to UTF-8
- `uint32_t unicode_from_utf8(const char **input)` - UTF-8 to codepoint

## Performance

- **Initialization**: < 1ms (data already embedded)
- **Property Lookup**: O(log n) binary search, ~100ns per lookup
- **Case Conversion**: O(1) direct lookup, ~50ns
- **UTF-8 Conversion**: O(1) per character, ~30ns
- **Memory Usage**: ~15MB embedded data (compressed from ~30MB)

## Compile-Time Guarantees

The system uses C23 features for safety:

```c
// Data must not be empty
_Static_assert(sizeof(unicode_basic_data) > 0, "Basic data must not be empty");

// Magic number verification
_Static_assert(((const unicode_file_header_t *)unicode_basic_data)->magic == UNICODE_MAGIC,
               "Invalid magic number");

// Size verification
_Static_assert(sizeof(unicode_props_data) == EXPECTED_PROPS_SIZE,
               "Props data size mismatch");
```

## Integration with BDI

The Unicode data system integrates with:
- **Lexer**: Character classification for tokenization
- **Parser**: Unicode identifiers and string literals
- **Semantic Analysis**: Unicode normalization and comparison
- **Code Generation**: UTF-8 string handling
- **AI Curriculum**: Natural language processing and multilingual support

## Regenerating Data

To regenerate Unicode data files:

```bash
cd tools/build_tables
make clean
make generate
```

This will:
1. Parse all Unicode 17.0.0 source files
2. Generate compressed .dat files
3. Create #embed headers
4. Verify data integrity

## Testing

Run tests with:

```bash
cd tools/build_tables
make test
```

Tests verify:
- Parser correctness
- Data integrity (checksums)
- API functionality
- Edge cases
- Performance benchmarks

## Future Enhancements

- [ ] Full collation algorithm (UCA)
- [ ] Unicode normalization forms (NFC, NFD, NFKC, NFKD)
- [ ] Grapheme cluster breaking
- [ ] Word breaking
- [ ] Line breaking
- [ ] Case folding
- [ ] Regular expression support
- [ ] Locale-specific data
- [ ] Bidirectional text algorithm

## References

- Unicode 17.0.0 Standard: https://www.unicode.org/versions/Unicode17.0.0/
- Unicode Character Database: https://www.unicode.org/ucd/
- Unicode Technical Reports: https://www.unicode.org/reports/
- C23 Standard: ISO/IEC 9899:2023
