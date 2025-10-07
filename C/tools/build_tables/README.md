
# Unicode Data Build Tools

This directory contains tools for parsing Unicode 17.0.0 data files and generating compressed binary data files for the BDI AI curriculum engine.

## Overview

The Unicode data pipeline consists of:
- **Parsers**: Extract data from Unicode text files
- **Compressors**: Compress data using RLE and dictionary encoding
- **Generators**: Create binary .dat files with headers and checksums
- **Embed Generator**: Create C23 #embed headers for compile-time inclusion

## Building

```bash
make                # Build the tools
make generate       # Generate all .dat files
make test          # Run tests
make clean         # Clean build artifacts
```

#

## Components

### make_unidata.c
Main orchestrator that:
- Coordinates all parsers
- Generates all .dat files
- Creates embed headers
- Reports statistics

### Parsers

#### parse_ucd.c
Parses Unicode Character Database files:
- UnicodeData.txt: Character properties, case mappings, decompositions
- PropList.txt: Character property lists
- Blocks.txt: Unicode block definitions
- Scripts.txt: Script assignments

Extracts:
- General Category
- Canonical Combining Class
- Bidi Class
- Decomposition Type and Mapping
- Numeric Values
- Case Mappings (upper, lower, title)
- Script and Block assignments

#### parse_emoji.c
Parses emoji data files:
- emoji-test.txt: Emoji test sequences
- emoji-sequences.txt: Emoji sequences
- emoji-zwj-sequences.txt: Zero-Width Joiner sequences

Extracts:
- Basic emoji
- Emoji sequences
- ZWJ sequences
- Skin tone modifiers
- Gender modifiers

#### parse_collation.c
Parses collation data:
- allkeys-17.0.0.txt: Default Unicode Collation Element Table

Extracts:
- Primary, secondary, tertiary weights for sorting
- Collation keys for all characters

#### parse_idna.c
Parses IDNA (Internationalized Domain Names in Applications) data:
- IdnaMappingTable.txt: IDNA character mappings
- IdnaTestV2.txt: IDNA test cases

Extracts:
- Valid/disallowed characters for domain names
- Character mappings for normalization
- IDNA status codes

#### parse_unihan.c
Parses Unihan (Han character) database:
- Unihan_Readings.txt: Pronunciation data
- Unihan_Variants.txt: Character variants
- Unihan_RadicalStrokeCounts.txt: Radical and stroke data

Extracts:
- Mandarin, Cantonese readings
- Japanese On/Kun readings
- Korean, Vietnamese readings
- Radical numbers and stroke counts
- Simplified/Traditional variants

### Compression

#### compress.c
Implements compression algorithms:
- **RLE (Run-Length Encoding)**: For consecutive identical bytes
- **Dictionary Compression**: For repeated patterns
- **Bit-packing**: For flags and small integers
- **Delta Encoding**: For sequential values

Also provides:
- CRC32 checksum calculation
- Compression/decompression utilities

### Embed Generation

#### embed_gen.c
Generates C23 #embed headers:
- Creates unicode_embed.h with #embed directives
- Adds compile-time assertions (_Static_assert)
- Validates data sizes and checksums
- Documents data format

## Data Format

Each .dat file has the following structure:

### Header (64 bytes)
```c
typedef struct {
    uint32_t magic;              // 0x42444955 ("BDIU")
    uint8_t version_major;       // 17
    uint8_t version_minor;       // 0
    uint8_t version_patch;       // 0
    uint8_t data_type;           // Type identifier
    uint32_t uncompressed_size;  // Original size
    uint32_t compressed_size;    // Compressed size
    uint32_t checksum;           // CRC32
    uint32_t num_entries;        // Entry count
    uint32_t index_offset;       // Index location
    uint32_t data_offset;        // Data location
    uint32_t reserved[4];        // Future use
} unicode_file_header_t;
```

### Index Section
- Array of ranges or offsets for fast lookup
- Binary search friendly

### Data Section
- Compressed character data
- Properties, mappings, sequences

## Generated Files

### unicode_basic.dat
Basic Unicode data:
- Character ranges
- Character classes (letters, digits, punctuation, etc.)
- Basic properties

### unicode_math.dat
Mathematical symbols:
- Operators (∀, ∃, ∈, ∉, ∩, ∪)
- Relations (=, ≠, <, >, ≤, ≥, ≡, ≈)
- Arrows (→, ←, ↔, ⇒, ⇐, ⇔)
- Greek letters (α, β, γ, δ)
- Special symbols (∞, ∂, ∇, ∫, ∑, ∏)

### unicode_props.dat
Full character properties:
- General Category
- Combining Class
- Bidi Class
- Decomposition
- Numeric Values
- Case Mappings
- Script and Block
- Age (version introduced)

### unicode_emoji.dat
Emoji data:
- Basic emoji
- Emoji sequences
- ZWJ sequences
- Modifiers (skin tone, gender)
- Emoji components

### unicode_collation.dat
Collation keys:
- Primary weights
- Secondary weights
- Tertiary weights
- Sorting order

### unicode_idna.dat
IDNA mappings:
- Valid characters
- Disallowed characters
- Character mappings
- Normalization rules

### unicode_han.dat
Unihan data:
- Pronunciation readings (Mandarin, Cantonese, Japanese, Korean, Vietnamese)
- Radical numbers
- Stroke counts
- Character variants (simplified/traditional)

## Usage Example

```c
#include "unicode_embed.h"
#include "unicode_tables.h"

// Access embedded data
const unicode_file_header_t *header = 
    (const unicode_file_header_t *)unicode_basic_data;

// Verify magic number
assert(header->magic == UNICODE_MAGIC);

// Lookup character properties
unicode_char_props_t props;
if (unicode_get_char_props(0x0041, &props)) {
    printf("Character: U+%04X\n", props.codepoint);
    printf("Category: %d\n", props.general_category);
    printf("Uppercase: U+%04X\n", props.uppercase);
}
```

## Compression Statistics

Target compression ratios:
- Basic data: 50-60% reduction
- Properties: 60-70% reduction
- Emoji: 40-50% reduction
- Collation: 55-65% reduction
- IDNA: 60-70% reduction
- Unihan: 50-60% reduction

## Testing

Run tests with:
```bash
make test
```

Tests verify:
- Parser correctness
- Data integrity (checksums)
- Compression/decompression round-trips
- File format compliance
- #embed integration

## Performance

Typical generation times (on modern hardware):
- UnicodeData.txt parsing: ~0.5s
- Emoji parsing: ~0.1s
- Collation parsing: ~1.0s
- IDNA parsing: ~0.2s
- Unihan parsing: ~0.8s
- Total pipeline: ~3-5s

## Dependencies

- C23 compiler (GCC 14+ or Clang 18+)
- Standard C library
- No external dependencies

## Future Enhancements

- [ ] More sophisticated compression (LZ77, Huffman)
- [ ] Incremental updates
- [ ] Memory-mapped file support
- [ ] Multi-threaded parsing
- [ ] Additional Unicode properties
- [ ] Locale-specific data
- [ ] Performance optimizations

## References

- Unicode 17.0.0 Standard: https://www.unicode.org/versions/Unicode17.0.0/
- Unicode Character Database: https://www.unicode.org/ucd/
- C23 Standard: ISO/IEC 9899:2023
