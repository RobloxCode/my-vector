/*
 * Tests for the customization macros (VEC_REALLOC, VEC_FREE, VEC_ASSERT,
 * VEC_INITIAL_CAP). They must be defined before including my_vector.h, which
 * is why this lives in its own translation unit.
 */

#include <stdio.h>
#include <stdlib.h>

static int reallocs = 0;
static int frees = 0;
static int fail_after =
    -1; /* fail every realloc once this many have happened */
static int assert_failures = 0;

static void *test_realloc(void *p, size_t n) {
    if (fail_after >= 0 && reallocs >= fail_after) {
        return NULL;
    }
    reallocs++;
    return realloc(p, n);
}

static void test_free(void *p) {
    if (p != NULL) {
        frees++;
    }
    free(p);
}

#define VEC_REALLOC      test_realloc
#define VEC_FREE         test_free
#define VEC_ASSERT(cond) ((cond) ? (void)0 : (void)assert_failures++)
#define VEC_INITIAL_CAP  4

#include "../my_vector.h"

VEC_DEFINE(long, LongVec)

static int checks = 0;
static int failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        checks++;                                                              \
        if (!(cond)) {                                                         \
            failures++;                                                        \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__,   \
                    #cond);                                                    \
        }                                                                      \
    } while (0)

static void test_custom_allocator_is_used(void) {
    LongVec v;
    LongVec_init(&v);

    CHECK(LongVec_push(&v, 1));
    CHECK(reallocs == 1 && v.cap == 4); /* VEC_INITIAL_CAP override */

    for (long i = 0; i < 20; i++) {
        LongVec_push(&v, i);
    }
    CHECK(reallocs > 1);

    LongVec_deinit(&v);
    CHECK(frees == 1);
}

static void test_failed_allocation_leaves_vector_unchanged(void) {
    LongVec v;
    LongVec_init(&v);
    for (long i = 0; i < 4; i++) {
        LongVec_push(&v, i);
    }

    long *buf = v.data;
    size_t cap = v.cap;
    size_t len = v.len;

    fail_after = reallocs; /* every further realloc fails */

    CHECK(!LongVec_push(&v, 99));
    CHECK(!LongVec_insert(&v, 0, 99));
    CHECK(!LongVec_reserve(&v, 100));
    CHECK(!LongVec_push_many(&v, buf, 1));
    CHECK(v.data == buf && v.cap == cap && v.len == len);

    LongVec_pop(&v, NULL);
    CHECK(!LongVec_shrink_to_fit(&v));
    CHECK(v.data == buf && v.cap == cap && v.len == len - 1);

    fail_after = -1;
    LongVec_deinit(&v);
}

static void test_assert_hook(void) {
    LongVec v;
    LongVec_init(&v);
    LongVec_push(&v, 1);
    LongVec_push(&v, 2);
    LongVec_pop(&v, NULL);

    int before = assert_failures;
    (void)LongVec_get(&v, 0); /* in bounds */
    CHECK(assert_failures == before);

    /* Index 1 is out of bounds but still inside the buffer (cap is 4), so
     * reading it is harmless here and lets us observe the assert firing. */
    (void)LongVec_get(&v, 1);
    CHECK(assert_failures == before + 1);

    LongVec_deinit(&v);
}

int main(void) {
    test_custom_allocator_is_used();
    test_failed_allocation_leaves_vector_unchanged();
    test_assert_hook();

    printf("%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
