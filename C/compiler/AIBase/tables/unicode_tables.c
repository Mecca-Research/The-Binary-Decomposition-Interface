
#include "unicode_tables.h"
#include "../embeddings/unicode_embed.h"
#include <string.h>
#include <stdlib.h>

// Global state
static bool initialized = false;
static const unicode_char_props_t *props_table = NULL;
static size_t props_count = 0;

// Initialize tables from embedded data
bool unicode_tables_init(void) {
    if (initialized) return true;
    
    // Verify embedded data
    const unicode_file_header_t *header = 
        (const unicode_file_header_t *)unicode_props_data;
    
    if (header->magic != UNICODE_MAGIC) {
        return false;
    }
    
    // Set up properties table
    props_table = (const unicode_char_props_t *)(unicode_props_data + sizeof(unicode_file_header_t));
    props_count = header->num_entries;
    
    initialized = true;
    return true;
}

// Cleanup
void unicode_tables_cleanup(void) {
    initialized = false;
    props_table = NULL;
    props_count = 0;
}

// Binary search for character properties
bool unicode_get_char_props(uint32_t codepoint, unicode_char_props_t *props) {
    if (!initialized || !props_table || !props) return false;
    
    size_t left = 0;
    size_t right = props_count;
    
    while (left < right) {
        size_t mid = left + (right - left) / 2;
        
        if (props_table[mid].codepoint == codepoint) {
            *props = props_table[mid];
            return true;
        } else if (props_table[mid].codepoint < codepoint) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    
    return false;
}

// Character class queries
bool unicode_is_letter(uint32_t codepoint) {
    unicode_char_props_t props;
    if (!unicode_get_char_props(codepoint, &props)) return false;
    return props.general_category <= 4; // Lu, Ll, Lt, Lm, Lo
}

bool unicode_is_digit(uint32_t codepoint) {
    unicode_char_props_t props;
    if (!unicode_get_char_props(codepoint, &props)) return false;
    return props.general_category >= 8 && props.general_category <= 10; // Nd, Nl, No
}

bool unicode_is_whitespace(uint32_t codepoint) {
    unicode_char_props_t props;
    if (!unicode_get_char_props(codepoint, &props)) return false;
    return props.general_category >= 22 && props.general_category <= 24; // Zs, Zl, Zp
}

bool unicode_is_punctuation(uint32_t codepoint) {
    unicode_char_props_t props;
    if (!unicode_get_char_props(codepoint, &props)) return false;
    return props.general_category >= 11 && props.general_category <= 17; // Pc-Po
}

bool unicode_is_symbol(uint32_t codepoint) {
    unicode_char_props_t props;
    if (!unicode_get_char_props(codepoint, &props)) return false;
    return props.general_category >= 18 && props.general_category <= 21; // Sm, Sc, Sk, So
}

bool unicode_is_math_symbol(uint32_t codepoint) {
    unicode_char_props_t props;
    if (!unicode_get_char_props(codepoint, &props)) return false;
    return props.general_category == 18; // Sm
}

// Case conversion
uint32_t unicode_to_upper(uint32_t codepoint) {
    unicode_char_props_t props;
    if (!unicode_get_char_props(codepoint, &props)) return codepoint;
    return props.uppercase;
}

uint32_t unicode_to_lower(uint32_t codepoint) {
    unicode_char_props_t props;
    if (!unicode_get_char_props(codepoint, &props)) return codepoint;
    return props.lowercase;
}

uint32_t unicode_to_title(uint32_t codepoint) {
    unicode_char_props_t props;
    if (!unicode_get_char_props(codepoint, &props)) return codepoint;
    return props.titlecase;
}

// Emoji queries (stub - needs emoji table)
bool unicode_is_emoji(uint32_t codepoint) {
    // TODO: Implement with emoji table
    return (codepoint >= 0x1F300 && codepoint <= 0x1F9FF);
}

bool unicode_get_emoji_data(uint32_t codepoint, unicode_emoji_t *emoji) {
    // TODO: Implement with emoji table
    (void)codepoint;
    (void)emoji;
    return false;
}

// Collation (stub - needs collation table)
bool unicode_get_collation_key(uint32_t codepoint, unicode_collation_key_t *key) {
    // TODO: Implement with collation table
    (void)codepoint;
    (void)key;
    return false;
}

int unicode_compare(uint32_t cp1, uint32_t cp2) {
    // Simple codepoint comparison for now
    if (cp1 < cp2) return -1;
    if (cp1 > cp2) return 1;
    return 0;
}

// IDNA (stub - needs IDNA table)
bool unicode_is_idna_valid(uint32_t codepoint) {
    // TODO: Implement with IDNA table
    (void)codepoint;
    return true;
}

bool unicode_get_idna_mapping(uint32_t codepoint, unicode_idna_mapping_t *mapping) {
    // TODO: Implement with IDNA table
    (void)codepoint;
    (void)mapping;
    return false;
}

// Unihan (stub - needs Unihan table)
bool unicode_get_unihan_data(uint32_t codepoint, unicode_unihan_t *unihan) {
    // TODO: Implement with Unihan table
    (void)codepoint;
    (void)unihan;
    return false;
}

// Normalization
size_t unicode_decompose(uint32_t codepoint, uint32_t *output, size_t output_size) {
    unicode_char_props_t props;
    if (!unicode_get_char_props(codepoint, &props)) {
        if (output_size > 0) output[0] = codepoint;
        return 1;
    }
    
    if (props.decomp_len == 0) {
        if (output_size > 0) output[0] = codepoint;
        return 1;
    }
    
    size_t len = props.decomp_len < output_size ? props.decomp_len : output_size;
    for (size_t i = 0; i < len; i++) {
        output[i] = props.decomp_mapping[i];
    }
    
    return props.decomp_len;
}

uint32_t unicode_compose(const uint32_t *input, size_t input_size) {
    // TODO: Implement composition
    (void)input;
    (void)input_size;
    return 0;
}

// UTF-8 utilities
size_t unicode_strlen(const char *str) {
    size_t len = 0;
    while (*str) {
        if ((*str & 0xC0) != 0x80) len++;
        str++;
    }
    return len;
}

int unicode_strcmp(const char *s1, const char *s2) {
    // TODO: Implement proper Unicode collation
    return strcmp(s1, s2);
}

char *unicode_to_utf8(uint32_t codepoint, char *output) {
    if (codepoint <= 0x7F) {
        output[0] = (char)codepoint;
        output[1] = '\0';
    } else if (codepoint <= 0x7FF) {
        output[0] = (char)(0xC0 | (codepoint >> 6));
        output[1] = (char)(0x80 | (codepoint & 0x3F));
        output[2] = '\0';
    } else if (codepoint <= 0xFFFF) {
        output[0] = (char)(0xE0 | (codepoint >> 12));
        output[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        output[2] = (char)(0x80 | (codepoint & 0x3F));
        output[3] = '\0';
    } else if (codepoint <= 0x10FFFF) {
        output[0] = (char)(0xF0 | (codepoint >> 18));
        output[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        output[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        output[3] = (char)(0x80 | (codepoint & 0x3F));
        output[4] = '\0';
    }
    return output;
}

uint32_t unicode_from_utf8(const char **input) {
    const unsigned char *str = (const unsigned char *)*input;
    uint32_t codepoint = 0;
    
    if (str[0] <= 0x7F) {
        codepoint = str[0];
        *input += 1;
    } else if ((str[0] & 0xE0) == 0xC0) {
        codepoint = ((str[0] & 0x1F) << 6) | (str[1] & 0x3F);
        *input += 2;
    } else if ((str[0] & 0xF0) == 0xE0) {
        codepoint = ((str[0] & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
        *input += 3;
    } else if ((str[0] & 0xF8) == 0xF0) {
        codepoint = ((str[0] & 0x07) << 18) | ((str[1] & 0x3F) << 12) | 
                    ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);
        *input += 4;
    }
    
    return codepoint;
}
