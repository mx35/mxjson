/*
 * ----------------------------------------------------------------------
 * |\ /| mxjson.h
 * | X | JSON Parser
 * |/ \| Copyright 2018 em@x35.co.uk
 * ----------------------------------------------------------------------
 */

#ifndef MXJSON_H
#define MXJSON_H

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>

#include "mxstr.h"
#include "mxutil.h"


/**
 * Types for JSON tokens
 *
 * A token is initially marked as MXJSON_NONE, and set to its actual type
 * once the type has been determined.
 */
typedef enum {
    MXJSON_NONE,
    MXJSON_NULL,
    MXJSON_BOOL,
    MXJSON_NUMBER,
    MXJSON_STRING,
    MXJSON_OBJECT,
    MXJSON_ARRAY,
    MXJSON_COUNT
} mxjson_type;


/**
 * Type to index into the token array
 *
 * Indices are used rather than pointers because the token array may be
 * resized.
 */
typedef uint32_t mxjson_idx_t;


/**
 * Reserved index value to indicate index not set.
 *
 * Note: The choice of 0 for MXJSON_IDX_NONE means that the first entry in
 * the token array does not contain a real token. It is a sentinel that
 * represents the parent of the first token.
 */
#define MXJSON_IDX_NONE 0


/**
 * Information about a parsed JSON value.
 *
 * A token may consist of just a value, or may have both a name and value
 * where it is a member of a JSON object. When the token does not have a
 * name, both name and name_size are set to 0.
 *
 * The strings represented by name/name_size and str/str_size are offsets
 * into the JSON being parsed.
 *
 * The value_type field indicates the type of value (mxjson_type), with
 * the value stored in the union:
 *
 *  - MXJSON_NONE: no value
 *  - MXJSON_NULL: no vaue
 *  - MXJSON_BOOL: boolean field
 *  - MXJSON_NUMBER: str and str_size represent the string value for the number
 *  - MXJSON_STRING: str and str_size represent the string value
 *  - MXJSON_OBJECT: children is the number of members of the object.
 *  - MXJSON_ARRAY: children is the number of members of the array.
 *
 * For MXJSON_OBJECT and MXJSON_ARRAY, the "next" field gives the index of
 * the token immediately following the object/array contents.
 *
 * The references to other tokens (parent and next) all use array index values
 * rather than pointers to accommodate the case where the token array is
 * resized.
 *
 * The name_esc and value_esc flags indicate whether any escape characters
 * (\", \\, \/, \b, \f, \n, \r, \t or \uxxxx) were encountered during parsing
 * of the name and (string) value respectively. If there are no escape
 * characters, the string from the JSON buffer may be used directly, otherwise
 * the non-escaped string can be obtained using mxjson_token_name and/or
 * mxjson_token_string.
 *
 * Note: The use of a bitfield for name_size/value_type etc. is to optimise the
 * memory usage for a token. The tradeoff is that the maximum length for an
 * object member name is 2^27.
 */
typedef struct {
    uint32_t     name;          /**< Offset into parse buffer for token name */
    uint32_t     name_size:27;  /**< length of token name */
    uint32_t     name_esc:1;    /**< Whether name contains escape characters */
    uint32_t     value_esc:1;   /**< Whether value contains escape chars */
    uint32_t     value_type:3;  /**< Type of token (mxjson_type) */
    mxjson_idx_t parent;        /**< Index for parent token */

    union {
        bool             boolean;  /**< Value for a boolean token */

        struct {
            uint32_t     str;      /**< Parse buffer index for string value */
            uint32_t     str_size; /**< Length of string value */
        };

        struct {
            uint32_t     children; /**< Number of children in array/object */
            mxjson_idx_t next;     /**< Next token after array/object */
        };
    };
} mxjson_token_t;


/**
 * Parser context.
 *
 * Contains an array of mxjson_token_t entries to represent the parsed JSON.
 */
typedef struct mxjson_parser_t mxjson_parser_t;


/**
 * Callback function type to allocate additional JSON tokens.
 *
 * A function of this type may be passed to mxjson_init() to manage
 * the memory allocation for token parsing. This callback is used to
 * allocate more tokens when the current token array is full, and to
 * free any allocated resources, by passing a size_hint of 0, when
 * mxjson_free() is called.
 *
 * When more tokens are required, this function should resize the
 * tokens array in the parser context -i.e.:
 *
 * 1. Allocate a new tokens array of the required size
 *
 * 2. Copy the existing token array into the new array (size of the
 *    existing array is given by the count field in the parser context).
 *
 * 3. Free any resources associated with the existing tokens array. Note:
 *    the existing array may be one previously allocated by the callback,
 *    or it may be a user-supplied array (equal to the init_tokens field
 *    in the parser context). The user-supplied array must not be freed.
 *
 * 4. Update the tokens and count fields in the parser context.
 *
 * The provided mxjson_resize function (see below) may be used to provide
 * malloc based management for the token array.
 *
 * On a resize operation, the callback function is not obliged to allocate
 * the exact size requested, but must perform an allocation that
 * meets the following conditions:
 *
 * - Allocates at least one more token than is currently allocated
 *
 * - The token array size must be >= 2 (to account for the use of a
 *   sentinel).
 *
 * It is not necessary for the callback function to memset the allocated
 * array memory to zero.
 *
 * @param[in] parser
 *   The parser context containing the token array.
 *
 * @param[in] size_hint
 *   Gives an indication of the number of tokens required:
 *   0 : No tokens are required, the function should free any allocated
 *       resources.
 *   non-zero : A suggested token array size to allocate. This is the
 *       first power of 2 larger than the current array size. The strategy
 *       of doubling the array size means that there are at most log(n)
 *       resize operations.
 *
 * @return
 *   Indicates whether the allocation is successful. true should always
 *   be returned when the size_hint is 0.
 */
typedef bool (*mxjson_resize_cb)(mxjson_parser_t *parser,
                                 mxjson_idx_t     size_hint);


/**
 * Parser context
 *
 * Contains the input and result of parsing a JSON file.
 *
 * The parser context is initialised using mxjson_init() and populated with
 * the parsed representation of a JSON file when mxjson_parse() is called.
 *
 * The parser context may be reused across multiple parse operations. When
 * the parser context is no longer required, any resources allocated during
 * parsing may be freed by calling mxjson_free():
 *
 *   mxjson_init(p)
 *   mxjson_parse(p)
 *   ...
 *   mxjson_parse(p)
 *   mxjson_free(p)
 *
 * After a call to mxjson_parse(), the following information is available
 * in the parser context:
 *
 * - json: String containing the input JSON.
 *
 * - unparsed: Any remaining unparsed input. On success, unparsed is an
 *       empty string. If unparsed is non-empty, it means that either invalid
 *       JSON was encountered, or the tokens array was filled.
 *
 * - idx: Index for the final token to be processed/populated.
 *       If (idx == count), it means that there was insufficient space in
 *       the token array to parse the JSON (and either there is no resize
 *       function, or the resize failed).
 *
 * - tokens: The array of parsed tokens, which occupy elements 1..idx
 *       of the tokens array.
 *
 * IMPORTANT NOTE: The first token for the parsed JSON is at index 1, not
 * index 0.
 *
 * The parsed tokens may be processed in depth-first order (i.e. the order
 * they appear in the JSON input) as follows:
 *
 *     mxjson_parser_t  p;
 *     mxjson_token_t  *t;
 *     bool             valid;
 *     int              i;
 *
 *     mxjson_init(&p, 0, NULL, mxjson_resize);
 *     valid = mxjson_parse(&p, mxstr(json_ptr, json_size));
 *
 *     if (valid) {
 *         for (i = 1; i <= p.idx; i++) {
 *             t = &p.tokens[i];
 *             // Process token t
 *         }
 *     }
 *
 *     mxjson_free(&p);
 *
 * A JSON value, including any of its children can be skipped over, by jumping
 * directly to the next token following the value:
 *
 *     mxjson_idx_t index;
 *
 *     t = &p.tokens[index];
 *     // Want to ignore t and any children
 *     index = mxjson_next(&p, index);
 *
 * The immediate children of an object or array value can be processed using:
 *
 *     mxjson_idx_t parent_index;
 *     mxjson_idx_t index;
 *     mxjson_idx_t last;
 *
 *     parent_index = <index for parent token>
 *     last = mxjson_next(&p, parent_index);
 *     index = mxjson_first(&p, parent_index);
 *
 *     while (index != last) {
 *         t = &p.tokens[index];
 *         // Process token t
 *         index = mxjson_next(&p, index);
 *     }
 *
 *     // index now points to first token after the JSON value at parent_index
 */
struct mxjson_parser_t {
    mxstr_t           json;     /**< The input JSON */
    mxstr_t           unparsed; /**< Remaining unparsed input */

   /**
    * The following fields track the token array.
    */
    mxjson_idx_t      idx;      /**< Index for last token processed */
    mxjson_idx_t      count;    /**< Current size of tokens array */
    mxjson_token_t   *tokens;   /**< Array of tokens */

    /**
     * The following fields are used to track the position in the JSON
     * hierarchy that has been reached during parsing. The token field
     * is a pointer to the current/last token to be populated, and the
     * current_parent field gives the index of the JSON object/array that
     * the current parse position is a child of.
     */
    mxjson_token_t   *token;          /**< Current token */
    mxjson_idx_t      current_parent; /**< Index for current parent token */

    /**
     * The following fields store information passed to mxjson_init().
     */
    mxjson_idx_t      init_count;  /**< Initial size for token array */
    mxjson_token_t   *init_tokens; /**< User supplied initial token array */
    mxjson_resize_cb  resize_fn;   /**< Callback for token array management */
};


/*
 * ----------------------------------------------------------------------
 * External API - Helper Functions
 * ----------------------------------------------------------------------
 */

/**
 * Callback function to allocate more functions.
 *
 * mxjson_resize() may be passed as the resize_fn parameter to mxjson_init()
 * to reallocate the token array during JSON parsing.
 *
 * Provides an implementation for mxjson_resize_cb().
 */
static inline bool mxjson_resize(mxjson_parser_t *p, mxjson_idx_t size_hint);


/**
 * Get the first child of a token.
 *
 * If the token has no children (i.e. it is not an array or object value,
 * or the array/object is empty), the next token after the specified idx
 * is returned.
 *
 * See mxjson_parser_t for example usage.
 *
 * @param[in] p
 *   The parser context.
 *
 * @param[in] idx
 *   The index of the current token.
 *
 * @return
 *   The index of the first child, or the index for next token after the
 *   current token if there are no children.
 */
static inline mxjson_idx_t mxjson_first(mxjson_parser_t *p, mxjson_idx_t idx);


/**
 * Get the index of the next token after a specified token.
 *
 * Returns the index for the first token after the specified token that is
 * not a descendent of the current token.
 *
 * Where the specified token is an object or array, the index for the first
 * token following the object/array is returned. If the specified token has no
 * children, the token immediately following the specified token is returned.
 *
 * See mxjson_parser_t for example usage.
 *
 * @param[in] p
 *   The parser context.
 *
 * @param[in] idx
 *   The index of the current token.
 *
 * @return
 *   The index of the next token.
 */
static inline mxjson_idx_t mxjson_next(mxjson_parser_t *p, mxjson_idx_t idx);


/**
 * Get the name string for a token.
 *
 * @param[in] p
 *   The parser context containing the tokens.
 *
 * @param[in] idx
 *   The index for the token whose name is required.
 *
 * @param[in] buffer
 *   A buffer to be used to store the unescaped version of the token
 *   name. This buffer is only used where necessary. If the name does
 *   not contain escaped characters, a reference to string within the JSON
 *   input stored in the parser context is returned
 *
 * @param[out] valid
 *   Indicates whether the token name has been successfully translated. If
 *   the token name contains invalid escape characters (e.g. unmatched
 *   UTF8 surrogate pair), valid is set to false, and the untranslated
 *   string from the JSON input is returned.
 *
 * @return
 *   A string containing the token name. An empty string is returned when the
 *   token does not represent a named JSON value. If the token name contains
 *   invalid escape characters, the unescaped string is returned, and valid
 *   is set to false.
 */
static inline mxstr_t mxjson_token_name(mxjson_parser_t *p,
                                        mxjson_idx_t     idx,
                                        mxbuf_t         *buffer,
                                        bool            *valid);


/**
 * Get the string value for a token.
 *
 * Note: this function may be used for any token type. Only MXJSON_STRING
 * tokens may contain escaped characters. MXJSON_OBJECT and MXJSON_ARRAY
 * token types return the strings "object" and "array" respectively.
 *
 * @param[in] p
 *   The parser context containing the tokens.
 *
 * @param[in] idx
 *   The index for the token whose value is required.
 *
 * @param[in] buffer
 *   A buffer to be used to store the unescaped version of the string
 *   value. This buffer is only used where necessary. If the value does not
 *   contain escaped characters, a reference to the string within the JSON
 *
 * @param[out] valid
 *   Indicates whether the token value has been successfully translated. If
 *   the token value contains invalid escape characters (e.g. unmatched
 *   UTF8 surrogate pair), valid is set to false, and the untranslated
 *   string from the JSON input is returned.
 *
 * @return
 *   A string containing the token value. If the token value contains
 *   invalid escape characters, the unescaped string is returned, and valid
 *   is set to false.
 */
static inline mxstr_t mxjson_token_string(mxjson_parser_t *p,
                                          mxjson_idx_t     idx,
                                          mxbuf_t         *buffer,
                                          bool            *valid);


/*
 * ----------------------------------------------------------------------
 * External API
 * ----------------------------------------------------------------------
 */

/**
 * Parse a JSON input.
 *
 * The typical flow for parsing is:
 *
 *     mxjson_parser_t p;
 *     bool            valid;
 *
 *     mxjson_init(&p, 0, NULL, mxjson_resize);
 *     valid = mxjson_parse(&p, mxstr(json_ptr, json_size));
 *     // Process the parse result in p
 *     mxjson_free(&p);
 *
 * @param[in] p
 *   The parser context. This must have been previously initialised by a
 *   call to mxjson_init.
 *
 * @param[in] json
 *   A string containing the JSON to parse.
 *
 * @return
 *   Indicates whether the parsing was successful. false is returned either
 *   when the JSON is invalid, or there were insufficient tokens in the
 *   parser context to complete the parsing.
 */
static inline bool mxjson_parse(mxjson_parser_t *p, mxstr_t json);


/**
 * Frees resources in the parser context.
 *
 * This function should be called once the parser context is no longer
 * required.
 *
 * Calling this function frees the resources allocated by the
 * mxjson_resize_cb() function, by calling the resize function with a size hint
 * of 0. It is the responsibility of the caller to release, if needed, the
 * memory for the parser context iself, and the tokens array passed to
 * mxjson_init().
 *
 * Note: It is good practice to always call this function, irrespective of
 * whether a resize function was passed to mxjson_init().
 *
 * @param[in] p
 *   The parser context.
 */
static inline void mxjson_free(mxjson_parser_t *p);


/**
 * Initialise a parser context.
 *
 * Before calling mxjson_parse to parse and validate a JSON input, a
 * parser context must be created to be used to store the results of the
 * parsing operation.
 *
 * The simplest usage, which is sufficient for the majority of use cases
 * is:
 *
 *     mxjson_parser_t p;
 *     mxjson_init(p, 0, NULL, mxjson_resize);
 *
 * The following examples describe alternate ways of specifying the
 * tokens array for the parser context:
 *
 * 1. Fixed size tokens array, no reallocation
 *
 *     mxjson_token_t tokens[100];
 *     mxjson_init(p, 100, tokens, NULL);
 *
 *    Uses the provided token array. Parsing fails if > 100 tokens are
 *    required.
 *
 * 2. Initial fixed size tokens array, with fallback to reallocation
 *
 *     mxjson_token_t tokens[100];
 *     mxjson_init(p, 100, tokens, mxjson_resize);
 *
 *    Uses the provided token array. mxjson_resize() is called if > 100
 *    tokens are required.
 *
 * 3. Use the reallocation callback with specified initial size.
 *
 *     mxjson_init(p, 1000, NULL, mxjson_resize);
 *
 *    mxjson_resize() is called (at parse time) to allocate an initial
 *    tokens array of size 1000. Further calls to mxjson_resize() are made
 *    if > 1000 tokens are required.
 *
 * 4. Custom allocation callback
 *
 *     mxjson_init(p, 100, NULL, custom_resize_fn);
 *
 *   A custom replacement for mxjson_resize() may be used.
 *   See mxjson_resize_cb() for details.
 *
 * @param[in] p
 *   Pointer to the parser context structure to initialize.
 *
 * @param[in] init_count
 *   The size of the tokens array. If NULL is passed for tokens,
 *   init_count is used to control the size of the initial tokens array
 *   allocation via the resize_fn.
 *
 * @param[in] tokens
 *   The tokens array to use to store the parsing result. NULL may be passed
 *   when a resize callback function is specified. If a tokens array and a
 *   resize callback function are specified, the caller supplied tokens
 *   array is used, with fallback to the resize function is the array isn't
 *   large enough.
 *
 * @param[in] resize_fn
 *   The resize function to used to allocate a larger tokens array when it
 *   is required. Typically, mxjson_resize() is passed for this parameter,
 *   unless a custom allocator is required (e.g. where malloc isn't available).
 */
static inline void mxjson_init(mxjson_parser_t  *p,
                               mxjson_idx_t      init_count,
                               mxjson_token_t   *tokens,
                               mxjson_resize_cb  resize_fn);


/*
 * ----------------------------------------------------------------------
 * Internal Implementation
 * ----------------------------------------------------------------------
 */

/**
 * \internal
 * Consume whitespace characters.
 *
 * Whitespace characters (specifically space, newline, carriage return
 * and tab) are consumed from the start of the string. The function returns
 * as soon as the string has non-whitespace as the first character.
 *
 * The bool return type is used so that the function can be called from
 * within a logical expression.
 *
 * @param[in,out] str
 *   The string to process. The string is updated to consume any JSON
 *   whitespace characters at the start of the string.
 *
 * @return
 *   Always returns true, indicating that zero or more whitespace characters
 *   have been consumed.
 */
static inline bool
mxjson_consume_ws (mxstr_t *str)
{
    mxstr_t s = *str;
    uint8_t c;

    mxstr_consume_chars(&s, &c, (c == ' ') || (c == '\n') ||
                        (c == '\r') || (c == '\t'));
    *str = s;

    return true;
}


/**
 * \internal
 * Parse a JSON number from the start of a string.
 *
 * @param[in,out] str
 *   The string to parse. The number is consumed from the start of the string.
 *   On error, characters are consumed up to the point the error is detected.
 *
 * @param[out] value
 *   On success, set to the string containing the representation of the
 *   parsed number. Not updated if an error is detected.
 *
 * @return
 *   Indicates whether the number was successfully parsed.
 */
static inline bool
mxjson_parse_number (mxstr_t *str, mxstr_t *value)
{
    mxstr_t s = *str;
    bool    ok;
    uint8_t c;

    /*
     * Optional '-' followed by at least one digit
     */
    (void)mxstr_consume_char(&s, &c, (c == '-'));
    ok = mxstr_consume_char(&s, &c, isdigit(c));

    if (ok) {
        /*
         * If the first digit is not '0', there may be zero or more
         * successive digits.
         */
        if (c != '0') {
            mxstr_consume_chars(&s, &c, isdigit(c));
        }

        /*
         * Optional '.' followed by one or more digits.
         */
        if (mxstr_consume_char(&s, &c, (c == '.'))) {
            ok = mxstr_consume_char(&s, &c, isdigit(c));
            mxstr_consume_chars(&s, &c, isdigit(c));
        }
    }

    /*
     * Optional 'e' or 'E' followed by optional '+' or '-' followed by
     * one or more digits.
     */
    if (ok && mxstr_consume_char(&s, &c, (c == 'e' || c == 'E'))) {
        (void)mxstr_consume_char(&s, &c, (c == '+' || c == '-'));
        ok = mxstr_consume_char(&s, &c, isdigit(c));
        mxstr_consume_chars(&s, &c, isdigit(c));
    }

    if (ok) {
        /*
         * Store the parsed value.
         */
        *value = mxstr_prefix(*str, s);
    }

    *str = s;

    return ok;
}


/**
 * \internal
 * Parse an escaped character
 *
 * This function handles the character (or characters in the case of \u)
 * immediately following a '\'.
 *
 * @param[in,out] str
 *   The string to parse. The escaped character is consumed from the start of
 *   the string. On error, characters are consumed up to the point the error
 *   is detected.
 *
 * @param[out] esc_flag
 *   Boolean flag, which is set if a valid escaped character is found.
 *
 * @return
 *   Indicates whether the escaped character is valid JSON.
 */
static inline bool
mxjson_parse_escaped_char (mxstr_t *str, bool *esc_flag)
{
    mxstr_t s = *str;
    bool    ok;
    uint8_t c;
    int     i;

    ok = mxstr_getchar(s, &c);

    if (ok) {
        switch (c) {
        case '\"': case '\\': case '/': case 'b':
        case 'f': case 'n': case 'r': case 't':
            (void)mxstr_consume(&s, 1);
            *esc_flag = true;
            break;

        case 'u':
            (void)mxstr_consume(&s, 1);

            for (i = 0; ok && i < 4; i++) {
                ok = mxstr_consume_char(&s, &c, isxdigit(c));
            }
            *esc_flag = true;
            break;

        default:
            ok = false;
            break;
        }
    }

    *str = s;

    return ok;
}


/**
 * \internal
 * Parse a JSON string value from the start of a string.
 *
 * @param[in,out] str
 *   The string to parse. The JSON string value is consumed from the start
 *   of the string. On error, characters are consumed up to the point the
 *   error is detected.
 *
 * @param[out] value
 *   On success, set to the string value. Not updated if an error is detected.
 *   Note: the string value returned still contains any escape sequences that
 *   were present.
 *
 * @param[out] esc_flag
 *   Set to indicate whether there are esccape sequences present in the
 *   string value.
 *
 * @return
 *   Indicates whether the string value was successfully parsed.
 */
static inline bool
mxjson_parse_string (mxstr_t *str, mxstr_t *value, bool *esc_flag)
{
    mxstr_t s = *str;
    mxstr_t start;
    bool    ok;
    uint8_t c = '\0';

    /*
     * Consume the opening quote character.
     */
    ok = mxstr_consume_char(&s, &c, (c == '\"')) && !mxstr_empty(s);
    start = s;

    /*
     * Continue consuming characters until either an invalid character
     * is encountered, or the closing quote is reached.
     */
    while (ok && mxstr_consume_char(&s, &c, (c >= ' ') && (c != '\"'))) {
        ok = (c != '\\' || mxjson_parse_escaped_char(&s, esc_flag));
    }

    if (ok && c == '\"') {
        /*
         * Get the string from the position immediately after the opening
         * quote to the current position (immediately before the closing
         * quote).
         */
        *value = mxstr_prefix(start, s);

        /*
         * Consume the closing quote character.
         */
        (void)mxstr_consume(&s, 1);
    } else {
        ok = false;
    }

    *str = s;

    return ok;
}


/**
 * \internal
 * Allocate a new token in the parser context.
 *
 * If no tokens are available, the tokens array is resized, provided a
 * resize callback function was passed to mxjson_init().
 *
 * The token is not returned explicitly. Instead, the token pointer in
 * the parser context is updated.
 *
 * @param[in] p
 *   The parser context.
 *
 * @return
 *   Indicates whether a token was successfully allocated.
 */
static inline bool
mxjson_token (mxjson_parser_t *p)
{
    mxjson_token_t *parent;
    mxjson_idx_t    idx;
    mxjson_token_t *token;
    bool            ok;

    ok = true;
    p->idx++;

    if (p->idx >= p->count) {
        /*
         * If the tokens array is currently NULL, perform the initial
         * token array allocation.
         */
        if (p->tokens == NULL) {

            if (p->init_tokens != NULL && p->idx < p->init_count) {
                /*
                 * Use the user supplied tokens array.
                 */
                p->tokens = p->init_tokens;
                p->count = p->init_count;

            } else {
                /*
                 * Call the resize function to perform an allocation.
                 */
                ok = (p->resize_fn != NULL &&
                      p->resize_fn(p, max(p->idx + 1, p->init_count)));
            }

            if (ok) {
                /*
                 * Clear the memory for the sentinel token at index 0.
                 */
                memset(&p->tokens[0], 0, sizeof(p->tokens[0]));
            }

        } else {
            /*
             * Resize the existing tokens array, if possible.
             */
            ok = (p->resize_fn != NULL &&
                  p->resize_fn(p, mxutil_size_p2(p->count)));
        }
    }

    if (ok) {
        idx = p->idx;
        assert(idx < p->count);
        token = &p->tokens[idx];
        memset(token, 0, sizeof(*token));
        token->parent = p->current_parent;
        parent = &p->tokens[p->current_parent];
        parent->children++;

        /*
         * Care is needed when storing pointer values to locations in the
         * tokens array, as the array may be resized. The token pointer
         * is always updated to a valid pointer following a successful
         * resize operation, and is never dereferenced following an
         * unsuccesful resize operation.
         */
        p->token = token;
    }

    return ok;
}


/**
 * \internal
 * Parse a JSON object member name string from the start of a string.
 *
 * The colon following the object member name is also parsed. The current
 * token in the parser context is updated.
 *
 * @param[in] p
 *   The parser context.
 *
 * @param[in,out] str
 *   The string to parse. The JSON object member name string is consumed from
 *   the start of the string. On error, characters are consumed up to the
 *   point the error is detected.
 *
 * @return
 *   Indicates whether the object member name was successfully parsed.
 */
static inline bool
mxjson_parse_name (mxjson_parser_t *p, mxstr_t *str)
{
    mxstr_t       s = *str;
    mxstr_t       name;
    unsigned char c;
    bool          esc_flag = false;
    bool          ok;

    ok = (mxjson_parse_string(&s, &name, &esc_flag) &&
          mxjson_consume_ws(&s) &&
          mxstr_consume_char(&s, &c, (c == ':')));

    if (ok) {
        p->token->name = mxstr_substr_offset(p->json, name);
        p->token->name_size = name.len;
        p->token->name_esc = esc_flag;
    }

    *str = s;

    return ok;
}


/**
 * \internal
 * Parse a JSON value from the start of a string
 *
 * The current token in the parser context is updated.
 *
 * @param[in] p
 *   The parser context.
 *
 * @param[in,out] str
 *   The string to parse. The JSON value is consumed from the start of the
 *   string, On error, characters are consumed up to the point the error is
 *   detected.
 *
 * @return
 *   Indicates whether a JSON value was successfully parsed.
 */
static inline bool
mxjson_parse_value (mxjson_parser_t *p,
                    mxstr_t         *str)
{
    mxstr_t s = *str;
    mxstr_t value;
    bool    ok;
    bool    esc_flag = false;
    uint8_t c;

    /*
     * The first character is used to identify the type of JSON value.
     */
    ok = mxstr_getchar(s, &c);

    if (ok) {
        switch (c) {
        case '\"':
            p->token->value_type = MXJSON_STRING;
            ok = mxjson_parse_string(&s, &value, &esc_flag);

            if (ok) {
                p->token->str = mxstr_substr_offset(p->json, value);
                p->token->str_size = value.len;
                p->token->value_esc = esc_flag;
            }
            break;

        case '{':
            (void)mxstr_consume(&s, 1);
            p->token->value_type = MXJSON_OBJECT;
            p->current_parent = p->idx;
            break;

        case '[':
            (void)mxstr_consume(&s, 1);
            p->token->value_type = MXJSON_ARRAY;
            p->current_parent = p->idx;
            break;

        case 't':
            p->token->value_type = MXJSON_BOOL;
            p->token->boolean = true;
            ok = mxstr_consume_str(&s, mxstr_literal("true"));
            break;

        case 'f':
            p->token->value_type = MXJSON_BOOL;
            p->token->boolean = false;
            ok = mxstr_consume_str(&s, mxstr_literal("false"));
            break;

        case 'n':
            p->token->value_type = MXJSON_NULL;
            ok = mxstr_consume_str(&s, mxstr_literal("null"));
            break;

        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            p->token->value_type = MXJSON_NUMBER;
            ok = mxjson_parse_number(&s, &value);

            if (ok) {
                p->token->str = mxstr_substr_offset(p->json, value);
                p->token->str_size = value.len;
            }
            break;

        default:
            ok = false;
            break;
        }
    }

    *str = s;

    return ok;
}


/**
 * \internal
 * Parse closing braces for arrays and objects
 *
 * @param[in] p
 *   The parser context.
 *
 * @param[in,out] str
 *   The string to parse. The closing braces, if any, for arrays (']') and
 *   objects ('}') along with any whitespace are consumed from the start of
 *   the string.
 *
 * @return
 *   The index for the enclosing array or object token for the current parse
 *   location, or MXJSON_IDX_NONE if the top level JSON value has been reached.
 */
static inline mxjson_idx_t
mxjson_ascend (mxjson_parser_t *p, mxstr_t *str)
{
    mxstr_t         s = *str;
    unsigned char   c;
    mxjson_idx_t    parent;
    bool            ascend = true;
    mxjson_token_t *token = NULL;

    parent = p->current_parent;

    if (parent != MXJSON_IDX_NONE) {
        do {
            token = &p->tokens[parent];
            mxjson_consume_ws(&s);

            if ((token->value_type == MXJSON_ARRAY &&
                 mxstr_consume_char(&s, &c, c == ']')) ||
                (token->value_type == MXJSON_OBJECT &&
                 mxstr_consume_char(&s, &c, c == '}'))) {
                token->next = p->idx + 1;
                parent = token->parent;

            } else {
                ascend = false;
            }

        } while (ascend);
    }

    *str = s;

    return parent;
}


/**
 * \internal
 * Parse and validate a JSON input.
 *
 * @param[in] p
 *   The parser context containing a reference to the JSON to parse.
 *
 * @return
 *   Indicates whether the parsing was successful. false is returned either
 *   when the JSON is invalid, or there were insufficient tokens in the
 *   parser context to complete the parsing.
 */
static inline bool
mxjson_parse_json (mxjson_parser_t *p)
{
    mxstr_t       s = p->unparsed;
    mxjson_idx_t  parent;
    unsigned char c;
    bool          ok;

    do {
        mxjson_consume_ws(&s);
        ok = mxjson_parse_value(p, &s);

        if (ok) {
            parent = mxjson_ascend(p, &s);
            p->current_parent = parent;

            if (parent != MXJSON_IDX_NONE) {
                /*
                 * Move to the next JSON value in the object/array.
                 * A ',' is expected if this isn't the first entry.
                 */
                mxjson_consume_ws(&s);
                ok = ((parent == p->idx) ||
                      mxstr_consume_char(&s, &c, c == ',')) && mxjson_token(p);

                /*
                 * Parse the name string and ':' for an object member.
                 */
                if (ok && p->tokens[parent].value_type == MXJSON_OBJECT) {
                    mxjson_consume_ws(&s);
                    ok = mxjson_parse_name(p, &s);
                }
            }
        }
    } while (ok && p->current_parent != MXJSON_IDX_NONE);

    if (ok) {
        /*
         * Reject the input if there is anything left to parse.
         */
        mxjson_consume_ws(&s);
        ok = mxstr_empty(s);
    }

    p->unparsed = s;

    return ok;
}


/**
 * \internal
 * Parse a 4-digit hex value
 *
 * @param[in] str
 *   The string to read from.
 *
 * @param[out] value
 *   The value parsed
 *
 * @return
 *   Indicates whether the start of the string contained a valid 4-digit
 *   hexadecimal value.
 */
static inline bool
mxjson_hex (mxstr_t *str, uint32_t *value)
{
    uint32_t v = 0;
    bool     ok = true;
    int      i;
    uint8_t  c;

    for (i = 0; ok && i < 4; i++) {
        ok = mxstr_consume_char(str, &c, isxdigit(c));

        if (ok) {
            c = (c > '9') ? (c & ~0x20) - 'A' + 10 : (c - '0');
            v = (v << 4) + c;
        }
    }

    *value = v;

    return ok;
}


/**
 * \internal
 * Translate a JSON string value, processing any escape characters.
 *
 * @param[in] buffer
 *   The buffer to write the translated string to.
 *
 * @param[in] str
 *   The JSON string to translate.
 *
 * @return
 *   Indicates whether the input string was valid and successfully translated.
 */
static inline bool
mxjson_unescape (mxbuf_t *buffer, mxstr_t str)
{
    mxstr_t  s = str;
    mxstr_t  start;
    bool     ok = true;
    uint8_t  c;
    uint32_t v1;
    uint32_t v2;

    while (ok && !mxstr_empty(s)) {
        start = s;
        mxstr_consume_chars(&s, &c, (c != '\\'));
        mxbuf_write(buffer, mxstr_prefix(start, s));
        (void)mxstr_consume(&s, 1);
        ok = mxstr_consume_char(&s, &c, true);

        if (ok) {
            switch (c) {
            case '\"': case '\\': case '/':
                (void)mxbuf_putc(buffer, c);
                break;

            case 'b':
                (void)mxbuf_putc(buffer, '\b');
                break;

            case 'f':
                (void)mxbuf_putc(buffer, '\f');
                break;

            case 'n':
                (void)mxbuf_putc(buffer, '\n');
                break;

            case 'r':
                (void)mxbuf_putc(buffer, '\r');
                break;

            case 't':
                (void)mxbuf_putc(buffer, '\t');
                break;

            case 'u':
                ok = mxjson_hex(&s, &v1);

                if (ok && v1 >= 0xd800 && v1 < 0xdc00) {
                    ok = (mxstr_consume_char(&s, &c, (c == '\\')) &&
                          mxstr_consume_char(&s, &c, (c == 'u')) &&
                          mxjson_hex(&s, &v2) && v2 >= 0xdc00 && v2 < 0xe000);

                    if (ok) {
                        v1 = 0x10000 + ((v1 - 0xd800) << 10) + (v2 - 0xdc00);
                    }
                }

                ok = ok && mxbuf_put_utf8(buffer, v1);
                break;

            default:
                ok = false;
                break;
            }
        }
    }

    return ok;
}


/*
 * ----------------------------------------------------------------------
 * API Implementation
 * ----------------------------------------------------------------------
 */

static inline bool
mxjson_resize (mxjson_parser_t *p, mxjson_idx_t size_hint)
{
    mxjson_token_t *tokens = NULL;

    if (size_hint != 0) {
        assert(size_hint > p->count);
        tokens = mxutil_calloc(size_hint * sizeof(*tokens));
        memcpy(tokens, p->tokens, p->count * sizeof(*tokens));
    }

    if (p->tokens != p->init_tokens) {
        free(p->tokens);
    }

    p->tokens = tokens;
    p->count = size_hint;

    return true;
}


static inline mxstr_t
mxjson_token_name (mxjson_parser_t *p,
                   mxjson_idx_t     idx,
                   mxbuf_t         *buffer,
                   bool            *valid)
{
    mxstr_t         s;
    mxjson_token_t *token;
    mxstr_t         str;
    bool            ok = true;

    token = &p->tokens[idx];
    str = mxstr((char *)&p->json.ptr[token->name], token->name_size);

    if (token->name_esc) {
        s = buffer->available;
        ok = mxjson_unescape(buffer, str);

        if (ok) {
            str = mxstr_prefix(s, buffer->available);
        }
    }

    if (valid != NULL) {
        *valid = ok;
    }

    return str;
}


static inline mxstr_t
mxjson_token_string (mxjson_parser_t *p,
                     mxjson_idx_t     idx,
                     mxbuf_t         *buffer,
                     bool            *valid)
{
    mxstr_t         s;
    mxjson_token_t *token;
    mxstr_t         str;
    bool            ok = true;

    token = &p->tokens[idx];

    switch (token->value_type) {
    case MXJSON_NULL:
        str = mxstr_literal("null");
        break;

    case MXJSON_BOOL:
        if (token->boolean) {
            str = mxstr_literal("true");
        } else {
            str = mxstr_literal("false");
        }
        break;

    case MXJSON_NUMBER:
    case MXJSON_STRING:
        str = mxstr((char *)&p->json.ptr[token->str], token->str_size);

        if (token->value_esc) {
            s = buffer->available;
            ok = mxjson_unescape(buffer, str);

            if (ok) {
                str = mxstr_prefix(s, buffer->available);
            }
        }
        break;

    case MXJSON_OBJECT:
        str = mxstr_literal("object");
        break;

    case MXJSON_ARRAY:
        str = mxstr_literal("array");
        break;

    default:
        str.ptr = NULL;
        str.len = 0;
        break;
    }

    if (valid != NULL) {
        *valid = ok;
    }

    return str;
}


static inline mxjson_idx_t
mxjson_first (mxjson_parser_t *p, mxjson_idx_t idx)
{
    UNUSED(p);

    return idx + 1;
}


static inline mxjson_idx_t
mxjson_next (mxjson_parser_t *p, mxjson_idx_t idx)
{
    mxjson_token_t *token;
    mxjson_idx_t    next;

    token = &p->tokens[idx];

    if (token->value_type == MXJSON_OBJECT ||
        token->value_type == MXJSON_ARRAY) {
        next = token->next;
    } else {
        next = idx + 1;
    }

    return next;
}


static inline bool
mxjson_parse (mxjson_parser_t *p, mxstr_t json)
{
    bool ok;

    p->json = json;
    p->unparsed = json;
    p->token = NULL;
    p->current_parent = MXJSON_IDX_NONE;
    p->idx = MXJSON_IDX_NONE;

    /*
     * Consume the optional UTF-8 BOM. This is not expected to be present,
     * but it is valid to accept it.
     */
    (void)mxstr_consume_str(&p->unparsed, mxstr_literal("\xEF\xBB\xBF"));

    /*
     * Get the root token and parse the JSON.
     */
    ok = (mxjson_token(p) && mxjson_parse_json(p));

    return ok;
}


static inline void
mxjson_free (mxjson_parser_t *p)
{
    bool ok;

    if (p->resize_fn != NULL) {
        ok = p->resize_fn(p, 0);
        assert(ok);
    }

    p->count = 0;
    p->tokens = NULL;
}


static inline void
mxjson_init (mxjson_parser_t  *p,
             mxjson_idx_t      init_count,
             mxjson_token_t   *tokens,
             mxjson_resize_cb  resize_fn)
{
    memset(p, 0, sizeof(*p));
    p->init_count = init_count;
    p->init_tokens = tokens;
    p->resize_fn = resize_fn;
}


#endif
