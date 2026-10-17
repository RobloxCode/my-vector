## my-vector.h

A generic dynamic array (vector) for C, built with macros. Single header, no dependencies beyond the standard library.

C has no templates, so `my_vector.h` gives you the next best thing: one macro, `VEC_DEFINE(T, Name)`, that generates a type-safe vector struct and all its functions for any element type. No `void *`, no casts, no generated source files, and everything is resolved at compile time.

## Features

- Works with any element type (ints, structs, typedef'd pointers, ...)
- Type-safe: each vector type has its own functions, so mixing types is a compile error
- Amortized O(1) push (capacity doubles when full)
- Allocation failures are reported through return values instead of crashing
- Overflow-checked capacity growth
- Header-only; every function is `static inline`

## Quick start

Copy `my_vector.h` into your project and include it:

```c
#include <stdio.h>
#include "my_vector.h"

VEC_DEFINE(int, IntVec)

int main(void)
{
    IntVec v;
    IntVec_init(&v);

    for (int i = 0; i < 10; i++) {
        if (!IntVec_push(&v, i * i)) {
            fprintf(stderr, "out of memory\n");
            return 1;
        }
    }

    for (size_t i = 0; i < v.len; i++) {
        printf("%d ", v.data[i]);
    }
    printf("\n");

    IntVec_free(&v);
    return 0;
}
```

Build with any C99 (or newer) compiler:

```sh
gcc -Wall -Wextra -std=c99 -o main main.c
```

## Using it with other types

```c
typedef struct {
    float x, y;
} Point;

typedef char *String; /* pointer types need a typedef first */

VEC_DEFINE(Point, PointVec)
VEC_DEFINE(String, StringVec)
```

Call `VEC_DEFINE` once per type. If several `.c` files need the same vector type, put the `VEC_DEFINE` line in a shared header.

## API

For `VEC_DEFINE(T, Name)`, the following are generated:

| Function | Description |
| --- | --- |
| `Name` | The vector struct: `data` (buffer), `len` (element count), `cap` (capacity) |
| `void Name_init(Name *v)` | Initialize an empty vector. Does not allocate. |
| `void Name_free(Name *v)` | Free the buffer and reset the vector. Safe to call twice. |
| `bool Name_reserve(Name *v, size_t want)` | Ensure capacity for at least `want` elements. |
| `bool Name_push(Name *v, T item)` | Append an element. |
| `bool Name_pop(Name *v, T *out)` | Remove the last element, optionally storing it in `out` (may be `NULL`). Returns `false` if empty. |
| `T *Name_at(Name *v, size_t i)` | Pointer to element `i`, or `NULL` if out of bounds. |
| `bool Name_insert(Name *v, size_t i, T item)` | Insert at index `i` (0 to `len`), shifting later elements right. |
| `bool Name_remove(Name *v, size_t i)` | Remove the element at `i`, shifting later elements left. |
| `void Name_clear(Name *v)` | Set the length to zero but keep the buffer. |

Functions returning `bool` return `false` on failure (out of memory, overflow, or an invalid index) and leave the vector unchanged.

You can also index the buffer directly with `v.data[i]` in hot loops, as long as `i < v.len`.

## Notes and caveats

- **Pointer types need a typedef.** The type name is pasted into function names, so `char *` must be wrapped as `typedef char *String;` before use.
- **Ownership.** The vector frees only its own buffer. If you store `malloc`'d pointers, free each one yourself before calling `Name_free`.
- **Pointer invalidation.** `Name_push`, `Name_insert` and `Name_reserve` may call `realloc`, which invalidates pointers from earlier `Name_at` calls or `v.data`.
- **Single token types.** `T` must be a single token (use a typedef for things like `unsigned int` or `struct foo *`).
- **Not thread-safe.** Synchronize access yourself if you share a vector between threads.

## How it works

`VEC_DEFINE` uses the preprocessor's token-pasting operator (`##`) to build unique names. For example, `Name##_push` with `Name` set to `IntVec` becomes `IntVec_push`. Each invocation expands into a full struct definition plus a set of `static inline` functions specialized for `T`, which is roughly what a C++ compiler does when it instantiates a template.

## Requirements

- A C99 compiler or newer (uses `stdbool.h`, `stdint.h`, and `static inline`) my-vector
