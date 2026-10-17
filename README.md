# my_vector.h

A generic dynamic array (vector) for C, built with macros. Single header, no dependencies beyond the standard library.

C has no templates, so `my_vector.h` gives you the next best thing: one macro, `VEC_DEFINE(T, Name)`, that generates a type-safe vector struct and all its functions for any element type. No `void *`, no casts, no generated source files, and everything is resolved at compile time.

## Features

- Works with any element type (ints, structs, pointers, ...)
- Type-safe: each vector type has its own functions, so mixing types is a compile error
- Amortized O(1) push (capacity doubles when full)
- Allocation failures are reported through return values instead of crashing, and the vector is left unchanged
- Overflow-checked capacity growth
- Optional custom allocator and assert hooks
- Named struct, so other headers can forward-declare the vector type
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

    IntVec_deinit(&v);
    return 0;
}
```

Build with any C99 (or newer) compiler:

```sh
gcc -Wall -Wextra -std=c99 -o main main.c
```

A larger example that uses every function is in [`example/example.c`](example/example.c) (`make example`).

## Using it with other types

```c
typedef struct {
    float x, y;
} Point;

VEC_DEFINE(Point, PointVec)
VEC_DEFINE(char *, StringVec)
```

Call `VEC_DEFINE` once per type, **without a trailing semicolon**. If several `.c` files need the same vector type, put the `VEC_DEFINE` line in a shared header.

## API

For `VEC_DEFINE(T, Name)`, these are generated (`v` is a `Name *`):

| Function | Description |
| --- | --- |
| `Name_init(v)` | Make `v` an empty vector. Does not allocate. |
| `Name_deinit(v)` | Free the buffer and reset to empty. Safe to call twice. |
| `bool Name_reserve(v, want)` | Ensure room for at least `want` elements. Never shrinks. |
| `bool Name_shrink_to_fit(v)` | Release unused capacity (frees the buffer if empty). |
| `bool Name_push(v, item)` | Append one element. |
| `bool Name_push_many(v, items, n)` | Append `n` elements copied from an array. |
| `bool Name_pop(v, out)` | Remove the last element; `out` may be `NULL`. `false` if empty. |
| `T *Name_at(v, i)` | Pointer to element `i`, or `NULL` if out of bounds. |
| `T Name_get(v, i)` | Element `i` by value. Asserts that `i` is in bounds. |
| `T *Name_last(v)` | Pointer to the last element, or `NULL` if empty. |
| `bool Name_insert(v, i, item)` | Insert so the item ends up at index `i` (`0..len`). |
| `bool Name_remove(v, i)` | Remove element `i`, keeping order. O(n). |
| `bool Name_swap_remove(v, i)` | Remove element `i` by moving the last one into its place. O(1), does not keep order. |
| `Name_clear(v)` | Set the length to zero but keep the buffer. |

Functions returning `bool` return `false` on failure (out of memory, size overflow, or an invalid index) and leave the vector unchanged. The fields `data` and `len` are public and can be read directly (`v->data[i]`, `v->len`); treat `cap` as read-only.

## Customization

Define these **before** including the header:

```c
#define VEC_REALLOC my_realloc   /* must behave like realloc, incl. realloc(NULL, n) */
#define VEC_FREE    my_free      /* define both or neither */
#define VEC_ASSERT(cond) my_assert(cond)  /* used by Name_get; default is assert */
#define VEC_INITIAL_CAP 16       /* first allocation size, default 8 */
#include "my_vector.h"
```

The allocator hooks are global to the program: there is no per-vector allocator or context pointer. `Name_get` bounds checks disappear when you build with `NDEBUG`.

## Forward declaring

The generated struct is named, so a header can refer to the vector without including the full definition:

```c
struct IntVec;                       /* in some other header */
size_t count_evens(const struct IntVec *v);
```

## Notes and caveats

- **Element types.** `T` can be any type you can write before a `*`, including pointer types like `char *`. Function pointer and array types need a typedef first.
- **Ownership.** The vector frees only its own buffer. If you store `malloc`'d pointers, free each one yourself before calling `Name_deinit`.
- **Pointer invalidation.** `Name_push`, `Name_push_many`, `Name_insert`, `Name_reserve` and `Name_shrink_to_fit` may call `realloc`, which invalidates pointers from earlier `Name_at` / `Name_last` calls or `v->data`. Do not pass `Name_push_many` a pointer into the same vector.
- **Not thread-safe.** Synchronize access yourself if you share a vector between threads.
- **Macro downsides.** Compiler errors point into the macro expansion and debuggers step through it less nicely than plain functions. That comes with this style of generic C.

## How it works

`VEC_DEFINE` uses the preprocessor's token-pasting operator (`##`) to build unique names. For example, `Name##_push` with `Name` set to `IntVec` becomes `IntVec_push`. Each invocation expands into a full struct definition plus a set of `static inline` functions specialized for `T`, which is roughly what a C++ compiler does when it instantiates a template.

## Tests

```sh
make test      # builds and runs the tests with AddressSanitizer + UBSan
make example   # builds and runs example/example.c
```

The tests cover every function, the overflow and out-of-memory paths (using a failing allocator through `VEC_REALLOC`), and a randomized comparison against a plain array.

## Requirements

- A C99 compiler or newer (uses `stdbool.h`, `stdint.h`, and `static inline`)
