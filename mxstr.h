/*
 * ----------------------------------------------------------------------
 * |\ /| mxstr.h
 * | X | String API
 * |/ \| Copyright 2018 em@x35.co.uk
 * ----------------------------------------------------------------------
 */

/* ----------------------------------------------------------------------
 * Note
 *
 * This version of the API contains a cut-down set of functionality
 * sufficient for supporting the mxjson parser.
 * ----------------------------------------------------------------------
 */

#ifndef MXSTR_H
#define MXSTR_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "mxutil.h"


/**
 * A string reference.
 *
 * A mxstr_t string is a reference to a block of memory of specified size
 * containing data. The data does not have to be a standard string:
 * - It does not need to be terminated with a '\0'.
 * - It may contain arbitrary values (e.g. not limited to printable
 *   characters).
 *
 * The string is a reference to the block of memory. The memory is not
 * considered owned by the mxstr_t instance. In general, the application
 * must maintain a separate reference to the memory for the purposes of
 * memory management.
 *
 * Where a string that owns the associated memory is required, the mxbuf_t
 * buffer type may be used.
 *
 * Since the string associated with a string reference is, in general,
 * not null terminated, care must be taken when using string references
 * with APIs that expect null termination. For printf functions, the
 * "%.*s" format string may be used:
 *
 *     mxstr_t str = ...;
 *     printf("%.*s\n", (int)str.len, str.ptr);
 *
 * Alternatively a null terminated string may be constructed:
 *
 *     mxstr_t str = ...;
 *     mxstr_t cstr;
 *     mxbuf_t buf;
 *
 *     mxbuf_create(&buf, NULL, 0);
 *     mxbuf_write(&buf, str);
 *     mxbuf_putc(&buf, '\0');
 *     cstr = mxbuf(&buf);
 *     ...
 *     mxbuf_free(&buf);
 */
typedef struct {
    unsigned char *ptr;         /**< Pointer to the data */
    size_t         len;         /**< Size of the data */
} mxstr_t;


/**
 * Create a string reference.
 */
static inline mxstr_t
mxstr(char *str, size_t len)
{
    mxstr_t s;

    s.ptr = (unsigned char *)str;
    s.len = len;

    return s;
}


/**
 * Create a mxstr_t for a string literal.
 *
 * The parameter passed to this literal must be either:
 * - A string literal e.g. "my string"
 * - A literal string declared as char literal[] = "my string"
 *
 * @param[in] str_
 *   A string literal.
 *
 * @return
 *   A mxstr_t string representing the string literal.
 */
#define mxstr_literal(str_) mxstr((str_), sizeof(str_) - 1)


/**
 * Test whether a string is empty
 *
 * A string is considered empty when it has length 0. (No matter what
 * the pointer value is.
 */
static inline bool
mxstr_empty(mxstr_t str)
{
    return (str.len == 0);
}


/*
 * ----------------------------------------------------------------------
 * Substring
 * ----------------------------------------------------------------------
 */

/**
 * Find the offset of a substring within a string.
 *
 * The start of the substring must be inside the block of memory
 * referenced by the string. The offset of the substring relative
 * to the start of the string is returned.
 */
static inline size_t
mxstr_substr_offset(mxstr_t str, mxstr_t substr)
{
    size_t idx;

    idx = (uintptr_t)substr.ptr - (uintptr_t)str.ptr;
    assert(idx <= str.len);

    return idx;
}


/**
 * Get the prefix of a substring.
 *
 * The prefix that is returned consists of the characters from the
 * start of the string up to, but not including, the first character of
 * the substring.
 */
static inline mxstr_t
mxstr_prefix(mxstr_t str, mxstr_t substr)
{
    mxstr_t s;

    s.ptr = str.ptr;
    s.len = mxstr_substr_offset(str, substr);

    return s;
}


/**
 * Get the substring of a string.
 *
 * The substring is specified using start and end indices (offsets relative
 * to the start of the string). The portion of the string that the falls
 * within the requested substring is returned.
 *
 * Note: An empty string is returned if none of the requested substring
 * overlaps with the string.
 *
 * @param[out] substr
 *   The requested substring.
 *
 * @return
 *   Indicates whether exactly the requested substring has been returned.
 *   false is returned when either the start or end index is greater than
 *   the length of the string, or the end index is smaller than the start
 *   index.
 */
static inline bool
mxstr_substr(mxstr_t str, size_t start, size_t end, mxstr_t *substr)
{
    bool ok = true;

    if (end < start) {
        end = start;
        ok = false;
    }

    if (start > str.len) {
        start = str.len;
        ok = false;
    }

    if (end > str.len) {
        end = str.len;
        ok = false;
    }

    substr->ptr = &str.ptr[start];
    substr->len = end - start;

    return ok;
}

/*
 * ----------------------------------------------------------------------
 * Comparison
 * ----------------------------------------------------------------------
 */

/**
 * Compare two strings.
 *
 * Equivalent to strcmp().
 */
static inline int
mxstr_cmp(mxstr_t str1, mxstr_t str2)
{
    int cmp;

    cmp = memcmp(str1.ptr, str2.ptr, min(str1.len, str2.len));

    if (cmp == 0) {
        cmp = str1.len - str2.len;
    }

    return cmp;
}


/*
 * ----------------------------------------------------------------------
 * Read
 * ----------------------------------------------------------------------
 */


/**
 * Get the first character from a string.
 *
 * The return value indicates whether a character was present.
 */
static inline bool
mxstr_getchar(mxstr_t str, unsigned char *c)
{
    bool ok;

    ok = (str.len > 0);

    if (ok) {
        *c = str.ptr[0];
    }

    return ok;
}


/**
 * Consume characters from the start of a string.
 *
 * @param[in,out] str
 *   The string. This is updated to remove the specified number of characters
 *   from the start of the string. If the string is shorter than the number of
 *   characters to remove, the empty string is returned.
 *
 * @param[in] len
 *   The number of characters to consume.
 *
 * @return
 *   The number of characters that were consumed.
 */
static inline size_t
mxstr_consume(mxstr_t *str, size_t len)
{
    size_t size;

    size = min(str->len, len);
    str->ptr = &str->ptr[size];
    str->len -= size;

    return size;
}


/**
 * Consume a matching character from the start of a string
 *
 * For example, to consume a digit character:

 *     ok = mxstr_consume_char(str, &c, isdigit(c));
 *     if (ok) {
 *         // c contains the matching digit character
 *     }
 *
 * @param[in,out] str_
 *   The string.
 *
 * @param[out] c_
 *   Auxiliary variable to store the character from the start of the string.
 *   Note: this variable is not set when the string is empty.
 *
 * @param[in] match_
 *   A boolean expression for the match condition. The first character from
 *   the string is only consumed if it matches this condition.
 *
 * @return
 *   Indicates whether a matching character was consumed.
 */
#define mxstr_consume_char(str_, c_, match_)                            \
    (mxstr_getchar(*(str_), (c_)) && (match_) &&                        \
     mxstr_consume((str_), 1) == 1)


/**
 * Consume 0 or more instances of matching characters from the start of a
 * string.
 *
 * For example, to remove any whitespace characters from the start of a
 * string:
 *
 *     mxstr_consume_chars(str, &c, isspace(c));
 *  * @param[in,out] str_
 *   The string to consume characters from.
 *
 * @param c_
 *   Auxiliary variable which stores the last character to be checked.
 *   Note: this variable is not set when the string is empty.
 *
 * @param match_
 *   A boolean expression for the match condition. Characters matching
 *   this condition are consumed from the start of the string until the
 *   first character of the string does not match the condition.
 */
#define mxstr_consume_chars(str_, c_, match_)                           \
    do {                                                                \
        while (mxstr_consume_char(str_, c_, match_));                   \
    } while(false)



/**
 * Consume a matching prefix from the start of a string.
 *
 * For example, to consume "<header>" from the start of the string:
 *
 *     ok = mxstr_consume_str(str, mxstr_literal("<header>"));
 *     if (ok) {
 *         // a header tag was found
 *     }
 *
 * @param[in,out] str
 *   The string.
 *
 * @param[in] prefix
 *   The string to match. If str has a matching prefix, it is consumed.
 *
 * @return
 *   Indicates whether a matching prefix was found and consumed.
 */
static inline bool
mxstr_consume_str(mxstr_t *str, mxstr_t prefix)
{
    mxstr_t substr;

    return (mxstr_substr(*str, 0, prefix.len, &substr) &&
            mxstr_cmp(substr, prefix) == 0 &&
            mxstr_consume(str, prefix.len) == prefix.len);
}


/*
 * ----------------------------------------------------------------------
 * Output
 * ----------------------------------------------------------------------
 */

/*
 * Note: Typically string output is performed using mxbuf_t buffers rather
 * than strings. The string based functions here are used by the buffer
 * based API to write data to a string.
 */

/**
 * Write a string.
 *
 * The destination string is updated so that it references any remaining space
 * once the string has been written. A separate reference to the string
 * buffer being written must be kept to allow the written string to be
 * retrieved:
 *
 *     char buffer[100];
 *     mxstr_t buf;
 *     mxstr_t available;
 *     mxstr_t str
 *
 *     buf = mxstr(buffer, sizeof(buffer));
 *     available = buf;
 *
 *     mxstr_write(&available, mxstr_literal("hello"));
 *     mxstr_write(&available, mxstr_literal(" ... goodbye"));
 *     str = mxstr_prefix(buf, available);
 *     // str contains "hello ... goodbye"
 *
 * @param[in,out] dest
 *   The string to write to. This is updated so that it references any
 *   remaining space not written to.
 *
 * @param[in] src
 *   The string to write.
 *
 * @return
 *   The number of characters that were written.
 */
static inline size_t
mxstr_write(mxstr_t *dest, mxstr_t src)
{
    size_t size;

    size = min(dest->len, src.len);
    memcpy(dest->ptr, src.ptr, size);
    dest->ptr = &dest->ptr[size];
    dest->len -= size;

    return size;
}


/**
 * Write repeated characters to a string.
 *
 * @param[in,out] dest
 *   The string to write to. This is updated so that it references any
 *   remaining space not written to.
 *
 * @param[in] c
 *   The character to write to the destination string.

 * @param[in] n
 *   The number of times to write the character.
 *
 * @return
 *   The number of characters that were written.
 */
static inline size_t
mxstr_write_chars(mxstr_t *dest, unsigned char c, size_t n)
{
    size_t size;

    size = min(dest->len, n);
    memset(dest->ptr, c, size);
    dest->ptr = &dest->ptr[size];
    dest->len -= size;

    return size;
}


/**
 * Write a single character to a string.
 *
 * @param[in,out] str
 *   The string to write to. This is updated so that it references any
 *   remaining space not written to.
 *
 * @param[in] c
 *   The character to write.
 *
 * @return
 *   Indicates whether there was space to write the character.
 */
static inline bool
mxstr_putc(mxstr_t *str, unsigned char c)
{
    bool ok;

    ok = !mxstr_empty(*str);

    if (ok) {
        str->ptr[0] = c;
        (void)mxstr_consume(str, 1);
    }

    return ok;
}


/*
 * ----------------------------------------------------------------------
 * Buffer
 * ----------------------------------------------------------------------
 */


/**
 * An output buffer.
 *
 * The buffer implementation resizes the buffer on demand if more space
 * is required.
 */
typedef struct {
    mxstr_t buf;       /**< The current buffer to write to */
    mxstr_t available; /**< The remaining space in the buffer */
    mxstr_t init;      /**< The caller supplied buffer space to use */
} mxbuf_t;


/**
 * Initialise a buffer
 *
 * The supplied buffer memory is used if possible. If additional space
 * is required a new block of memory is allocated to use for the buffer.
 * It is valid to pass a zero-length block of memory - a new block of memory
 * is allocated as soon as any space is required.
 *
 * @param[in] buffer
 *   The buffer to initialise
 *
 * @param[in] ptr
 *   Pointer to the buffer memory to use.
 *
 * @param[in] len
 *   Size of the buffer memory.
 */
static inline void
mxbuf_create(mxbuf_t *buffer, void *ptr, size_t len)
{
    mxstr_t str = mxstr(ptr, len);

    buffer->buf = str;
    buffer->available = str;
    buffer->init = str;
}


/**
 * Reset a buffer to be empty.
 *
 * Any data written to the buffer is discarded.
 *
 * @param[in] buffer
 *   The buffer to reset.
 */
static inline void
mxbuf_reset(mxbuf_t *buffer)
{
    buffer->available = buffer->buf;
}


/**
 * Free the allocated memory associated with a buffer.
 *
 * The buffer is reset back to use the block of memory passed to
 * mxbuf_create()
 *
 * @param[in] buffer
 *   The buffer.
 */
static inline void
mxbuf_free(mxbuf_t *buffer)
{
    if (buffer->buf.ptr != buffer->init.ptr) {
        free(buffer->buf.ptr);
    }

    buffer->buf = buffer->init;
    buffer->available = buffer->init;
}


/**
 * Resize a buffer to remove any unused space.
 *
 * The resize is only performed when the current buffer has been allocated
 * by the buffer implementation. If the buffer is using the memory block
 * passed to mxbuf_create(), it is not modified.
 *
 * @param[in] buffer
 *   The buffer.
 */
static inline void
mxbuf_trim (mxbuf_t *buffer)
{
    mxstr_t str;
    size_t  len;

    if (buffer->buf.ptr != buffer->init.ptr) {
        len = mxstr_substr_offset(buffer->buf, buffer->available);
        str = mxstr(mxutil_realloc(buffer->buf.ptr, len), len);
        buffer->buf = str;
        mxstr_substr(str, len, len, &buffer->available);
    }
}


/**
 * Ensure there is space available in a buffer.
 *
 * The buffer is resized if required.
 *
 * @param[in] buffer
 *   The buffer.
 *
 * @param[in] size
 *   The additional space required.
 */
static inline void
mxbuf_require(mxbuf_t *buffer, size_t size)
{
    size_t   new_size;
    mxstr_t  str;
    size_t   len;
    void    *ptr = NULL;

    if (buffer->available.len < size) {
        new_size = max(mxutil_size_p2(buffer->buf.len), mxutil_size_p2(size));
        len = mxstr_substr_offset(buffer->buf, buffer->available);

        if (buffer->buf.ptr != buffer->init.ptr) {
            ptr = buffer->buf.ptr;
        }

        str = mxstr(mxutil_realloc(ptr, new_size), new_size);
        buffer->buf = str;
        mxstr_substr(str, len, new_size, &buffer->available);
    }
}


/**
 * Write a string to a buffer.
 *
 * The buffer is resized if necessary.
 *
 * @param[in] buffer
 *   The buffer to write to.
 *
 * @param[in] str
 *   The string to write to the buffer.
 *
 * @return
 *   The number of characters written (i.e. str.len)
 */
static inline size_t
mxbuf_write(mxbuf_t *buffer, mxstr_t str)
{
    mxbuf_require(buffer, str.len);

    return mxstr_write(&buffer->available, str);
}


/**
 * Write a character to a buffer
 *
 * The buffer is resized if necessary.
 *
 * @param[in] buffer
 *   The buffer to write to.
 *
 * @param[in] c
 *   The character to write.
 *
 * @return
 *   Indicates whether the character was written successfully. (i.e. always
 *   returns true).
 */
static inline bool
mxbuf_putc(mxbuf_t *buffer, unsigned char c)
{
    mxbuf_require(buffer, 1);

    return mxstr_putc(&buffer->available, c);
}


/**
 * Write a UTF-8 encoded unicode codepoint to a buffer,
 *
 * The buffer is resized if necessary.
 *
 * @param[in] buffer
 *   The buffer to write to.
 *
 * @param[in] c
 *   The codepoint to write. This must be in the rang 0..0x110000
 *
 * @return
 *   Indicates whether the codepoint was successfully written. false is
 *   returned if the codepoint is out of range.
 */
static inline bool
mxbuf_put_utf8(mxbuf_t *buffer, uint32_t c)
{
    bool ok = true;

    if (c < 0x80) {
        (void)mxbuf_putc(buffer, c);
    } else if (c < 0x800) {
        (void)mxbuf_putc(buffer, 0xc0 + ((c >> 6) & 0x1f));
        (void)mxbuf_putc(buffer, 0x80 + (c & 0x3f));
    } else if (c < 0x10000) {
        (void)mxbuf_putc(buffer, 0xe0 + ((c >> 12) & 0xf));
        (void)mxbuf_putc(buffer, 0x80 + ((c >> 6) & 0x3f));
        (void)mxbuf_putc(buffer, 0x80 + (c & 0x3f));
    } else if (c < 0x110000) {
        (void)mxbuf_putc(buffer, 0xf0 + ((c >> 18) & 0x7));
        (void)mxbuf_putc(buffer, 0x80 + ((c >> 12) & 0x3f));
        (void)mxbuf_putc(buffer, 0x80 + ((c >> 6) & 0x3f));
        (void)mxbuf_putc(buffer, 0x80 + (c & 0x3f));
    } else {
        ok = false;
    }

    return ok;
}


/**
 * Write repeated characters to a buffer.
 *
 * The buffer is resized if necessary.
 *
 * @param[in] buffer
 *   The buffer to write to.
 *
 * @param[in] c
 *   The character to write.

 * @param[in] n
 *   The number of times to write the character.
 *
 * @return
 *   The number of characters that were written (i.e. n).
 */
static inline size_t
mxbuf_write_chars(mxbuf_t *buffer, unsigned char c, size_t n)
{
    mxbuf_require(buffer, n);

    return mxstr_write_chars(&buffer->available, c, n);
}


/**
 * Get a string reference for a buffers contents.
 *
 * @param buffer
 *   The buffer.
 *
 * @return
 *   The string.
 */
static inline mxstr_t
mxbuf_str(mxbuf_t *buffer)
{
    return mxstr_prefix(buffer->buf, buffer->available);
}


#endif
