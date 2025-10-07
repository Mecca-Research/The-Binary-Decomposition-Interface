
#ifndef UNICODE_TYPES_H
#define UNICODE_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

// Magic number for BDI Unicode data files: "BDIU"
#define UNICODE_MAGIC 0x42444955

// Unicode version
#define UNICODE_VERSION_MAJOR 17
#define UNICODE_VERSION_MINOR 0
#define UNICODE_VERSION_PATCH 0

// Data type identifiers
#define UNICODE_TYPE_BASIC      0x01
#define UNICODE_TYPE_MATH       0x02
#define UNICODE_TYPE_PROPS      0x03
#define UNICODE_TYPE_EMOJI      0x04
#define UNICODE_TYPE_COLLATION  0x05
#define UNICODE_TYPE_IDNA       0x06
#define UNICODE_TYPE_HAN        0x07

// Maximum code point
#define UNICODE_MAX_CODEPOINT 0x10FFFF

// File header structure
typedef struct {
    uint32_t magic;              // Magic number (UNICODE_MAGIC)
    uint8_t version_major;       // Unicode version major
    uint8_t version_minor;       // Unicode version minor
    uint8_t version_patch;       // Unicode version patch
    uint8_t data_type;           // Data type identifier
    uint32_t uncompressed_size;  // Original data size
    uint32_t compressed_size;    // Compressed data size
    uint32_t checksum;           // CRC32 checksum
    uint32_t num_entries;        // Number of entries
    uint32_t index_offset;       // Offset to index section
    uint32_t data_offset;        // Offset to data section
    uint32_t reserved[4];        // Reserved for future use
} unicode_file_header_t;

// Character range structure
typedef struct {
    uint32_t start;              // Start code point
    uint32_t end;                // End code point (inclusive)
    uint32_t data_offset;        // Offset to data for this range
} unicode_range_t;

// Character properties
typedef struct {
    uint32_t codepoint;
    uint8_t general_category;    // General Category
    uint8_t combining_class;     // Canonical Combining Class
    uint8_t bidi_class;          // Bidirectional Class
    uint8_t decomp_type;         // Decomposition Type
    int32_t numeric_value;       // Numeric Value (-1 if none)
    uint32_t uppercase;          // Uppercase mapping
    uint32_t lowercase;          // Lowercase mapping
    uint32_t titlecase;          // Titlecase mapping
    uint32_t decomp_mapping[18]; // Decomposition mapping (max 18 chars)
    uint8_t decomp_len;          // Length of decomposition
    uint8_t script;              // Script
    uint8_t block;               // Block
    uint8_t age;                 // Unicode version introduced
} unicode_char_props_t;

// General Categories
typedef enum {
    GC_Lu = 0,  // Letter, Uppercase
    GC_Ll,      // Letter, Lowercase
    GC_Lt,      // Letter, Titlecase
    GC_Lm,      // Letter, Modifier
    GC_Lo,      // Letter, Other
    GC_Mn,      // Mark, Nonspacing
    GC_Mc,      // Mark, Spacing Combining
    GC_Me,      // Mark, Enclosing
    GC_Nd,      // Number, Decimal Digit
    GC_Nl,      // Number, Letter
    GC_No,      // Number, Other
    GC_Pc,      // Punctuation, Connector
    GC_Pd,      // Punctuation, Dash
    GC_Ps,      // Punctuation, Open
    GC_Pe,      // Punctuation, Close
    GC_Pi,      // Punctuation, Initial quote
    GC_Pf,      // Punctuation, Final quote
    GC_Po,      // Punctuation, Other
    GC_Sm,      // Symbol, Math
    GC_Sc,      // Symbol, Currency
    GC_Sk,      // Symbol, Modifier
    GC_So,      // Symbol, Other
    GC_Zs,      // Separator, Space
    GC_Zl,      // Separator, Line
    GC_Zp,      // Separator, Paragraph
    GC_Cc,      // Other, Control
    GC_Cf,      // Other, Format
    GC_Cs,      // Other, Surrogate
    GC_Co,      // Other, Private Use
    GC_Cn       // Other, Not Assigned
} general_category_t;

// Math symbol types
typedef enum {
    MATH_OPERATOR = 0,
    MATH_RELATION,
    MATH_ARROW,
    MATH_GREEK,
    MATH_SPECIAL
} math_symbol_type_t;

// Emoji data
typedef struct {
    uint32_t codepoint;
    uint8_t emoji_type;          // Basic, modifier, component
    uint8_t skin_tone;           // Skin tone modifier
    uint8_t gender;              // Gender modifier
    bool is_zwj_sequence;        // Is ZWJ sequence
    uint32_t sequence[10];       // Emoji sequence
    uint8_t sequence_len;        // Length of sequence
} unicode_emoji_t;

// Collation key
typedef struct {
    uint32_t codepoint;
    uint32_t primary;            // Primary weight
    uint32_t secondary;          // Secondary weight
    uint32_t tertiary;           // Tertiary weight
} unicode_collation_key_t;

// IDNA mapping
typedef struct {
    uint32_t codepoint;
    uint8_t status;              // valid, ignored, mapped, deviation, disallowed
    uint32_t mapping[8];         // Mapped code points
    uint8_t mapping_len;         // Length of mapping
} unicode_idna_mapping_t;

// Unihan data
typedef struct {
    uint32_t codepoint;
    char mandarin[16];           // Mandarin reading
    char cantonese[16];          // Cantonese reading
    char japanese_on[16];        // Japanese On reading
    char japanese_kun[16];       // Japanese Kun reading
    char korean[16];             // Korean reading
    char vietnamese[16];         // Vietnamese reading
    uint8_t radical;             // Radical number
    uint8_t strokes;             // Stroke count
    uint32_t simplified;         // Simplified variant
    uint32_t traditional;        // Traditional variant
} unicode_unihan_t;

// Compression context
typedef struct {
    uint8_t *input;
    size_t input_size;
    uint8_t *output;
    size_t output_size;
    size_t output_capacity;
} compress_ctx_t;

// Parser context
typedef struct {
    const char *filename;
    FILE *file;
    char line[4096];
    int line_number;
    void *data;
    size_t data_count;
    size_t data_capacity;
} parser_ctx_t;

// Function prototypes
uint32_t crc32(const uint8_t *data, size_t len);
bool compress_rle(compress_ctx_t *ctx);
bool compress_dict(compress_ctx_t *ctx);
bool decompress_rle(compress_ctx_t *ctx);
bool decompress_dict(compress_ctx_t *ctx);

#endif // UNICODE_TYPES_H

