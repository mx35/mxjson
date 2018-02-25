/*
 * ----------------------------------------------------------------------
 * |\ /| mxutil.h
 * | X | Utilities
 * |/ \|
 * ----------------------------------------------------------------------
 */

#ifndef MXUTIL_H
#define MXUTIL_H

#include <assert.h>
#include <stdlib.h>


/**
 * Calculate the size of an array.
 */
#define mxarray_size(array_)  (sizeof(array_)/sizeof(*array_))


/**
 * Perform a compile time check and generate a compile error if the
 * check fails.
 *
 * The check is implemented by declaring an enumeration value with value 
 * 1 when the check passes, or a division by zero error when the check fails.
 *
 * See http://www.pixelbeat.org/programming/gcc/static_assert.html
 *
 * @param[in] name_
 *   A unqiue name for the check being performed. This is used to construct
 *   a name for the enumeration value.
 *
 * @param[in] condition_
 *   The condition to check. This must be an expression that is evaluatable
 *   at compile time.
 */
#define mxstatic_assert(name_, condition_)                              \
    ;enum { mxstatic_assert_ ## name_ = 1/(int)(!!(condition_)) }


/**
 * Declare a value as unused.
 *
 * Typically used to suppress unused parameter warnings.
 */
#define UNUSED(x_) (void)(x_)


/**
 * Compute the minimum of two values.
 *
 * Note: Argument expressions are evaluated twice, so care is needed
 * with expressions that have side effects.
 */
#define min(arg1_, arg2_)  ((arg1_) < (arg2_) ? (arg1_) : (arg2_))


/**
 * Compute the maximum of two values.
 *
 * Note: Argument expressions are evaluated twice, so care is needed
 * with expressions that have side effects.
 */
#define max(arg1_, arg2_)  ((arg1_) > (arg2_) ? (arg1_) : (arg2_))


/**
 * Find the smallest power of 2 that is larger than the input parameter.
 *
 * This function is typically used when growing array or buffer sizes.
 *
 * @param[in] value
 *   Find the smallest power of 2 larger than this value.
 *
 * @return
 *   The first power of 2 larger than value.
 */
static inline uint32_t
mxutil_size_p2(uint32_t value)
{
    return 1 << (32 - __builtin_clz(value));
}


/**
 * Allocate a block of memory.
 *
 * NULL is never returned.
 *
 * @param[in] size
 *   The size of memory to malloc.
 *
 * @return
 *   Pointer to an allocated block of memory of the requested size.
 */
static inline void *
mxutil_malloc(size_t size)
{
    void *ptr;

    ptr = malloc(size);
    assert(ptr != NULL);

    return ptr;
}


/**
 * Allocate and zero a block of memory.
 *
 * NULL is never returned.
 *
 * @param[in] size
 *   The size of memory to calloc.
 *
 * @return
 *   Pointer to an allocated block of memory of the requested size.
 */
static inline void *
mxutil_calloc(size_t size)
{
    void *ptr;

    ptr = calloc(1, size);
    assert(ptr != NULL);

    return ptr;
}



/**
 * Reallocate a block of memory.
 *
 * NULL is never returned.
 *
 * @param[in] ptr
 *   Pointer to the block of memory to resize. NULL may be passed.
 *
 * @param[in] size
 *   The new size for the memory. 0 must not be passed.
 *
 * @return
 *   Pointer to a reallocated block of memory of the requested size.
 */
static inline void *
mxutil_realloc(void *data, size_t size)
{
    void *ptr;

    ptr = realloc(data, size);
    assert(ptr != NULL);

    return ptr;
}


#endif
