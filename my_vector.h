#ifndef MY_VEC_H
#define MY_VEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VEC_INITIAl_CAP 8

#define VEC_DEFINE(T, Name)                                                    \
    typedef struct {                                                           \
        T *data;                                                               \
        size_t cap;                                                            \
        size_t len;                                                            \
    } Name;                                                                    \
                                                                               \
    static inline void Name##_init(Name v);                                    \
    static inline void Name##_free(Name *v);

#endif
