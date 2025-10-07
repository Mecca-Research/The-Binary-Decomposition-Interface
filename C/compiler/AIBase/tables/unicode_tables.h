
#ifndef UNICODE_TABLES_H
#define UNICODE_TABLES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations from unicode_types.h
typedef struct unicode_char_props unicode_char_props_t;
typedef struct unicode_emoji unicode_emoji_t;
typedef struct unicode_collation_key unicode_collation_key_t;
typedef struct unicode_idna_mapping unicode_idna_mapping_t;
typedef struct unicode_unihan unicode_unihan_t;

// Initialize Unicode tables from embedded data
bool unicode_tables_init(void);

// Cleanup Unicode tables
void unicode_tables_cleanup(void);

// Character property lookup
bool unicode_get_char_props(uint32_t codepoint, unicode_char_props_t *props);

// Character class queries
bool unicode_is_letter(uint32_t codepoint);
bool unicode_is_digit(uint32_t codepoint);
bool unicode_is_whitespace(uint32_t codepoint);
bool unicode_is_punctuation(uint32_t codepoint);
bool unicode_is_symbol(uint32_t codepoint);
bool unicode_is_math_symbol(uint32_t codepoint);

// Case conversion
uint32_t unicode_to_upper(uint32_t codepoint);
uint32_t unicode_to_lower(uint32_t codepoint);
uint32_t unicode_to_title(uint32_t codepoint);

// Emoji queries
bool unicode_is_emoji(uint32_t codepoint);
bool unicode_get_emoji_data(uint32_t codepoint, unicode_emoji_t *emoji);

// Collation
bool unicode_get_collation_key(uint32_t codepoint, unicode_collation_key_t *key);
int unicode_compare(uint32_t cp1, uint32_t cp2);

// IDNA
bool unicode_is_idna_valid(uint32_t codepoint);
bool unicode_get_idna_mapping(uint32_t codepoint, unicode_idna_mapping_t *mapping);

// Unihan (Han character data)
bool unicode_get_unihan_data(uint32_t codepoint, unicode_unihan_t *unihan);

// Normalization
size_t unicode_decompose(uint32_t codepoint, uint32_t *output, size_t output_size);
uint32_t unicode_compose(const uint32_t *input, size_t input_size);

// String operations
size_t unicode_strlen(const char *str);
int unicode_strcmp(const char *s1, const char *s2);
char *unicode_to_utf8(uint32_t codepoint, char *output);
uint32_t unicode_from_utf8(const char **input);

#endif // UNICODE_TABLES_H
