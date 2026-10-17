/*
 * vec.h - generic dynamic array (vector) for C, built with macros.
 *
 * Notes:
 *     - Functions that can allocate return false when memory runs out.
 *       The vector is left unchanged in that case.
 *     - Pointer types must be given a typedef first (typedef char *String;)
 *       because the type name is pasted into the generated function names.
 *     - The vector never frees what the elements point to. If you store
 *       malloc'd pointers, free each one yourself before calling Name_free.
 *     - Any function that grows the vector may call realloc, which
 *       invalidates pointers previously obtained from Name_at or data.
 *     - Call VEC_DEFINE once per type. Put it in a header if several .c
 *       files need the same vector type.
 */

#ifndef VEC_H
#define VEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Capacity allocated the first time a vector grows from empty. */
#define VEC_INITIAL_CAP 8

/*
 * VEC_DEFINE - generate a vector type and its functions.
 *
 * T    element type (must be a single token, see notes above)
 * Name name of the generated vector type, also used as the function prefix
 */
#define VEC_DEFINE(T, Name)                                                    \
    /* The vector itself: data is NULL while cap is 0. */                      \
    typedef struct {                                                           \
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
    static inline void Name##_free(Name *v) {                                  \
        free(v->data);                                                         \
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
        T *p = realloc(v->data, cap * sizeof(T));                              \
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
    /* Append item at the end. Returns false if memory ran out. */             \
    static inline bool Name##_push(Name *v, T item) {                          \
        if (!Name##_reserve(v, v->len + 1)) {                                  \
            return false;                                                      \
        }                                                                      \
                                                                               \
        v->data[v->len++] = item;                                              \
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
    /* Set the length to zero but keep the allocated buffer for reuse. */      \
    static inline void Name##_clear(Name *v) {                                 \
        v->len = 0;                                                            \
    }

#endif
