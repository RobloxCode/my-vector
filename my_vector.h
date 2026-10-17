/*
 * vec.h - generic dynamic array (vector) for C, built with macros.
 *
 * Usage:
 *     VEC_DEFINE(int, IntVec)     (no trailing semicolon)
 *     VEC_DEFINE(char *, StrVec)
 *
 *     IntVec v;
 *     IntVec_init(&v);
 *     IntVec_push(&v, 42);
 *     IntVec_deinit(&v);
 *
 * Notes:
 *     - Functions that can allocate return false when memory runs out or a
 *       size computation would overflow. The vector is left unchanged then.
 *     - T can be any type you can write before a '*', including pointer types
 *       like char *. Function pointer and array types need a typedef first.
 *     - Name is used as the struct tag and as the function prefix. Because the
 *       struct is named, other headers can say "struct Name;" and pass around
 *       pointers to it without seeing the full definition.
 *     - The vector never frees what the elements point to. If you store
 *       malloc'd pointers, free each one yourself before calling Name_deinit.
 *     - Any function that grows the vector may call realloc, which
 *       invalidates pointers previously obtained from Name_at, Name_last or
 *       v->data. This includes Name_push_many: do not pass it a pointer that
 *       points into the same vector.
 *     - Call VEC_DEFINE once per type per translation unit. Put it in a header
 *       if several .c files need the same vector type.
 *     - Allocator: define VEC_REALLOC and VEC_FREE before including this file
 *       to use your own (both or neither). They must behave like realloc and
 *       free, including realloc(NULL, n) acting as malloc.
 *     - Bounds checks: Name_get asserts that the index is valid. Define
 *       VEC_ASSERT(cond) before including this file to override, or build
 *       with NDEBUG to compile it out. Name_at returns NULL instead.
 *     - Requires C99 or later.
 */

#ifndef VEC_H
#define VEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(VEC_REALLOC) != defined(VEC_FREE)
    #error "Define both VEC_REALLOC and VEC_FREE, or neither."
#endif

#ifndef VEC_REALLOC
    #define VEC_REALLOC realloc
    #define VEC_FREE    free
#endif

#ifndef VEC_ASSERT
    #include <assert.h>
    #define VEC_ASSERT(cond) assert(cond)
#endif

/* Capacity allocated the first time a vector grows from empty. */
#ifndef VEC_INITIAL_CAP
    #define VEC_INITIAL_CAP 8
#endif

/*
 * VEC_DEFINE - generate a vector type and its functions.
 *
 * T    element type (see notes above)
 * Name name of the generated vector type, also used as the function prefix
 */
#define VEC_DEFINE(T, Name)                                                    \
    /* The vector itself: data is NULL while cap is 0. */                      \
    typedef struct Name {                                                      \
        T *data;    /* heap buffer holding the elements */                     \
        size_t len; /* number of elements currently stored */                  \
        size_t cap; /* number of elements the buffer can hold */               \
    } Name;                                                                    \
                                                                               \
    /* Initialize v as an empty vector. Does not allocate. */                  \
    static inline void Name##_init(Name *v) {                                  \
        v->data = NULL;                                                        \
        v->len = 0;                                                            \
        v->cap = 0;                                                            \
    }                                                                          \
                                                                               \
    /* Free the buffer and reset v to the empty state. Safe to call twice. */  \
    static inline void Name##_deinit(Name *v) {                                \
        VEC_FREE(v->data);                                                     \
        Name##_init(v);                                                        \
    }                                                                          \
                                                                               \
    /*                                                                         \
     * Make sure v can hold at least want elements without reallocating.       \
     * Capacity doubles on growth. Returns false on overflow or when           \
     * realloc fails, in which case v is unchanged.                            \
     */                                                                        \
    static inline bool Name##_reserve(Name *v, size_t want) {                  \
        if (want <= v->cap) {                                                  \
            return true;                                                       \
        }                                                                      \
                                                                               \
        size_t cap = v->cap ? v->cap : VEC_INITIAL_CAP;                        \
                                                                               \
        while (cap < want) {                                                   \
            if (cap > SIZE_MAX / 2) {                                          \
                return false;                                                  \
            }                                                                  \
            cap *= 2;                                                          \
        }                                                                      \
                                                                               \
        if (cap > SIZE_MAX / sizeof(T)) {                                      \
            return false;                                                      \
        }                                                                      \
                                                                               \
        T *p = VEC_REALLOC(v->data, cap * sizeof(T));                          \
        if (p == NULL) {                                                       \
            return false;                                                      \
        }                                                                      \
                                                                               \
        v->data = p;                                                           \
        v->cap = cap;                                                          \
                                                                               \
        return true;                                                           \
    }                                                                          \
                                                                               \
    /*                                                                         \
     * Release unused capacity so that cap == len (frees the buffer if the     \
     * vector is empty). Returns false if realloc fails, in which case v is    \
     * unchanged and still valid.                                              \
     */                                                                        \
    static inline bool Name##_shrink_to_fit(Name *v) {                         \
        if (v->len == v->cap) {                                                \
            return true;                                                       \
        }                                                                      \
                                                                               \
        if (v->len == 0) {                                                     \
            Name##_deinit(v);                                                  \
            return true;                                                       \
        }                                                                      \
                                                                               \
        T *p = VEC_REALLOC(v->data, v->len * sizeof(T));                       \
        if (p == NULL) {                                                       \
            return false;                                                      \
        }                                                                      \
                                                                               \
        v->data = p;                                                           \
        v->cap = v->len;                                                       \
                                                                               \
        return true;                                                           \
    }                                                                          \
                                                                               \
    /* Append item at the end. Returns false if memory ran out. */             \
    static inline bool Name##_push(Name *v, T item) {                          \
        if (!Name##_reserve(v, v->len + 1)) {                                  \
            return false;                                                      \
        }                                                                      \
                                                                               \
        v->data[v->len++] = item;                                              \
                                                                               \
        return true;                                                           \
    }                                                                          \
                                                                               \
    /*                                                                         \
     * Append n elements copied from items. n may be 0 (items is then          \
     * ignored). Returns false if memory ran out. items must not point into v  \
     * itself.                                                                 \
     */                                                                        \
    static inline bool Name##_push_many(Name *v, T const *items, size_t n) {   \
        if (n == 0) {                                                          \
            return true;                                                       \
        }                                                                      \
                                                                               \
        if (n > SIZE_MAX - v->len || !Name##_reserve(v, v->len + n)) {         \
            return false;                                                      \
        }                                                                      \
                                                                               \
        memcpy(&v->data[v->len], items, n * sizeof(T));                        \
        v->len += n;                                                           \
                                                                               \
        return true;                                                           \
    }                                                                          \
                                                                               \
    /*                                                                         \
     * Remove the last element. If out is not NULL, the removed element is     \
     * stored there. Returns false if the vector is empty.                     \
     */                                                                        \
    static inline bool Name##_pop(Name *v, T *out) {                           \
        if (v->len == 0) {                                                     \
            return false;                                                      \
        }                                                                      \
                                                                               \
        v->len--;                                                              \
                                                                               \
        if (out != NULL) {                                                     \
            *out = v->data[v->len];                                            \
        }                                                                      \
                                                                               \
        return true;                                                           \
    }                                                                          \
                                                                               \
    /* Pointer to element i, or NULL if i is out of bounds. */                 \
    static inline T *Name##_at(Name *v, size_t i) {                            \
        return i < v->len ? &v->data[i] : NULL;                                \
    }                                                                          \
                                                                               \
    /* Element i by value. Asserts (VEC_ASSERT) that i is in bounds. */        \
    static inline T Name##_get(const Name *v, size_t i) {                      \
        VEC_ASSERT(i < v->len);                                                \
                                                                               \
        return v->data[i];                                                     \
    }                                                                          \
                                                                               \
    /* Pointer to the last element, or NULL if the vector is empty. */         \
    static inline T *Name##_last(Name *v) {                                    \
        return v->len ? &v->data[v->len - 1] : NULL;                           \
    }                                                                          \
                                                                               \
    /*                                                                         \
     * Insert item so that it ends up at index i, shifting later elements      \
     * right. Valid indices are 0..len (len appends). Returns false if i is    \
     * out of range or memory ran out.                                         \
     */                                                                        \
    static inline bool Name##_insert(Name *v, size_t i, T item) {              \
        if (i > v->len || !Name##_reserve(v, v->len + 1)) {                    \
            return false;                                                      \
        }                                                                      \
                                                                               \
        memmove(&v->data[i + 1], &v->data[i], (v->len - i) * sizeof(T));       \
        v->data[i] = item;                                                     \
        v->len++;                                                              \
                                                                               \
        return true;                                                           \
    }                                                                          \
                                                                               \
    /*                                                                         \
     * Remove the element at index i, shifting later elements left.            \
     * Returns false if i is out of bounds.                                    \
     */                                                                        \
    static inline bool Name##_remove(Name *v, size_t i) {                      \
        if (i >= v->len) {                                                     \
            return false;                                                      \
        }                                                                      \
                                                                               \
        memmove(&v->data[i], &v->data[i + 1], (v->len - i - 1) * sizeof(T));   \
        v->len--;                                                              \
                                                                               \
        return true;                                                           \
    }                                                                          \
                                                                               \
    /*                                                                         \
     * Remove the element at index i in O(1) by moving the last element into   \
     * its place. Does not preserve order. Returns false if i is out of        \
     * bounds.                                                                 \
     */                                                                        \
    static inline bool Name##_swap_remove(Name *v, size_t i) {                 \
        if (i >= v->len) {                                                     \
            return false;                                                      \
        }                                                                      \
                                                                               \
        v->data[i] = v->data[v->len - 1];                                      \
        v->len--;                                                              \
                                                                               \
        return true;                                                           \
    }                                                                          \
                                                                               \
    /* Set the length to zero but keep the allocated buffer for reuse. */      \
    static inline void Name##_clear(Name *v) {                                 \
        v->len = 0;                                                            \
    }

#endif
