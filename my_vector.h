#ifndef MY_VEC_H
#define MY_VEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VEC_INIT_CAP 8

#define VEC_DEFINE(T, Name)                                                    \
    typedef struct {                                                           \
        T *data;                                                               \
        size_t cap;                                                            \
        size_t len;                                                            \
    } Name;                                                                    \
                                                                               \
    static inline void Name##_init(Name *v) {                                  \
        v->data = NULL;                                                        \
        v->cap = 0;                                                            \
        v->len = 0;                                                            \
    }                                                                          \
                                                                               \
    static inline void Name##_free(Name *v) {                                  \
        free(v);                                                               \
        Name##_init(v);                                                        \
    }                                                                          \
                                                                               \
    static inline bool Name##_reserve(Name *v, size_t want) {                  \
        if (v->cap >= want) {                                                  \
            return true;                                                       \
        }                                                                      \
                                                                               \
        size_t cap = v->cap ? v->cap : VEC_INIT_CAP;                           \
                                                                               \
        while (cap < want) {                                                   \
            if (cap > SIZE_MAX / 2) {                                          \
                return false;                                                  \
            }                                                                  \
                                                                               \
            cap *= 2;                                                          \
        }                                                                      \
                                                                               \
        if (cap > SIZE_MAX / sizeof(T)) {                                      \
            return false;                                                      \
        }                                                                      \
                                                                               \
        T *p = realloc(v->data, cap * sizeof(*p));                             \
        if (!p) {                                                              \
            return false;                                                      \
        }                                                                      \
                                                                               \
        v->data = p;                                                           \
        v->cap = cap;                                                          \
                                                                               \
        return true;                                                           \
    }                                                                          \
                                                                               \
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
    static inline bool Name##_pop(Name *v, T *out) {                           \
        if (v->len == 0) {                                                     \
            return false;                                                      \
        }                                                                      \
                                                                               \
        v->len--;                                                              \
                                                                               \
        if (out) {                                                             \
            *out = v->data[v->len];                                            \
        }                                                                      \
                                                                               \
        returt true;                                                           \
    }                                                                          \
                                                                               \
    static inline T Name##_at(Name *v, size_t i) {                             \
        return i < v->len ? &v->data[i] : NULL;                                \
    }                                                                          \
                                                                               \
    static inline Name##_clear(Name *v) {                                      \
        v->len = 0;                                                            \
    }

#endif
