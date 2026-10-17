#include "../my_vector.h"

#include <stdio.h>
#include <stdlib.h>

VEC_DEFINE(int, IntVec)
VEC_DEFINE(char *, StrVec)

typedef struct {
    float x, y;
} Point;

VEC_DEFINE(Point, PointVec)

static int checks = 0;
static int failures = 0;

/* Unlike assert, this keeps working under NDEBUG and keeps going on failure. */
#define CHECK(cond)                                                            \
    do {                                                                       \
        checks++;                                                              \
        if (!(cond)) {                                                         \
            failures++;                                                        \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__,   \
                    #cond);                                                    \
        }                                                                      \
    } while (0)

static void test_init_and_empty(void) {
    IntVec v;
    IntVec_init(&v);

    CHECK(v.data == NULL && v.len == 0 && v.cap == 0);
    CHECK(IntVec_at(&v, 0) == NULL);
    CHECK(IntVec_last(&v) == NULL);
    CHECK(!IntVec_pop(&v, NULL));
    CHECK(!IntVec_remove(&v, 0));
    CHECK(!IntVec_swap_remove(&v, 0));

    IntVec_deinit(&v);
}

static void test_push_and_growth(void) {
    IntVec v;
    IntVec_init(&v);

    CHECK(IntVec_push(&v, 0));
    CHECK(v.cap == VEC_INITIAL_CAP);

    for (int i = 1; i < 1000; i++) {
        CHECK(IntVec_push(&v, i));
    }

    CHECK(v.len == 1000 && v.cap >= 1000);
    for (int i = 0; i < 1000; i++) {
        CHECK(v.data[i] == i);
    }

    IntVec_deinit(&v);
}

static void test_push_own_element(void) {
    IntVec v;
    IntVec_init(&v);

    for (int i = 0; i < VEC_INITIAL_CAP; i++) {
        IntVec_push(&v, i);
    }

    /* The vector is exactly full, so this push reallocates. */
    CHECK(v.len == v.cap);
    CHECK(IntVec_push(&v, v.data[3]));
    CHECK(IntVec_get(&v, v.len - 1) == 3);

    IntVec_deinit(&v);
}

static void test_pop(void) {
    IntVec v;
    IntVec_init(&v);
    IntVec_push(&v, 1);
    IntVec_push(&v, 2);

    int out = 0;
    CHECK(IntVec_pop(&v, &out) && out == 2 && v.len == 1);
    CHECK(IntVec_pop(&v, NULL) && v.len == 0);
    CHECK(!IntVec_pop(&v, &out));

    IntVec_deinit(&v);
}

static void test_access(void) {
    IntVec v;
    IntVec_init(&v);
    IntVec_push(&v, 10);
    IntVec_push(&v, 20);

    CHECK(*IntVec_at(&v, 0) == 10);
    CHECK(*IntVec_at(&v, 1) == 20);
    CHECK(IntVec_at(&v, 2) == NULL);
    CHECK(IntVec_get(&v, 1) == 20);
    CHECK(*IntVec_last(&v) == 20);

    *IntVec_at(&v, 0) = 11; /* at gives a writable pointer */
    CHECK(v.data[0] == 11);

    IntVec_deinit(&v);
}

static void test_insert(void) {
    IntVec v;
    IntVec_init(&v);

    CHECK(IntVec_insert(&v, 0, 5));  /* into empty */
    CHECK(IntVec_insert(&v, 0, 1));  /* front */
    CHECK(IntVec_insert(&v, 2, 9));  /* append (i == len) */
    CHECK(IntVec_insert(&v, 1, 3));  /* middle */
    CHECK(!IntVec_insert(&v, 5, 0)); /* i > len */
    CHECK(v.len == 4);

    int expected[] = {1, 3, 5, 9};
    for (size_t i = 0; i < 4; i++) {
        CHECK(v.data[i] == expected[i]);
    }

    IntVec_deinit(&v);
}

static void test_remove(void) {
    IntVec v;
    IntVec_init(&v);
    for (int i = 0; i < 5; i++) {
        IntVec_push(&v, i);
    }

    CHECK(IntVec_remove(&v, 0)); /* 1 2 3 4 */
    CHECK(IntVec_remove(&v, 1)); /* 1 3 4 */
    CHECK(IntVec_remove(&v, 2)); /* 1 3 (last element) */
    CHECK(!IntVec_remove(&v, 2));
    CHECK(v.len == 2 && v.data[0] == 1 && v.data[1] == 3);

    IntVec_deinit(&v);
}

static void test_swap_remove(void) {
    IntVec v;
    IntVec_init(&v);
    for (int i = 0; i < 5; i++) {
        IntVec_push(&v, i);
    }

    CHECK(IntVec_swap_remove(&v, 1)); /* 0 4 2 3 */
    CHECK(v.len == 4 && v.data[1] == 4);
    CHECK(IntVec_swap_remove(&v, 3)); /* last: 0 4 2 */
    CHECK(v.len == 3 && v.data[2] == 2);
    CHECK(!IntVec_swap_remove(&v, 3));

    IntVec_deinit(&v);
}

static void test_push_many(void) {
    IntVec v;
    IntVec_init(&v);

    int items[50];
    for (int i = 0; i < 50; i++) {
        items[i] = i * 2;
    }

    CHECK(IntVec_push_many(&v, NULL, 0) && v.len == 0 && v.data == NULL);
    CHECK(IntVec_push_many(&v, items, 50) && v.len == 50);
    CHECK(IntVec_push_many(&v, items, 50) && v.len == 100);
    CHECK(v.data[49] == 98 && v.data[50] == 0 && v.data[99] == 98);
    CHECK(!IntVec_push_many(&v, items, SIZE_MAX)); /* len + n overflows */
    CHECK(v.len == 100);

    IntVec_deinit(&v);
}

static void test_reserve(void) {
    IntVec v;
    IntVec_init(&v);

    CHECK(IntVec_reserve(&v, 100) && v.cap >= 100 && v.len == 0);

    int *buf = v.data;
    for (int i = 0; i < 100; i++) {
        IntVec_push(&v, i);
    }
    CHECK(v.data == buf); /* no reallocation while within reserved capacity */

    size_t cap = v.cap;
    CHECK(IntVec_reserve(&v, 10) && v.cap == cap); /* never shrinks */

    /* Overflow paths must fail and leave the vector untouched. */
    CHECK(!IntVec_reserve(&v, SIZE_MAX));
    CHECK(!IntVec_reserve(&v, SIZE_MAX / 2));
    CHECK(!IntVec_reserve(&v, SIZE_MAX / sizeof(int) + 1));
    CHECK(v.data == buf && v.cap == cap && v.len == 100);

    IntVec_deinit(&v);
}

static void test_shrink_to_fit(void) {
    IntVec v;
    IntVec_init(&v);

    CHECK(IntVec_shrink_to_fit(&v)); /* empty and unallocated */
    CHECK(v.data == NULL);

    for (int i = 0; i < 20; i++) {
        IntVec_push(&v, i);
    }
    CHECK(IntVec_shrink_to_fit(&v) && v.cap == 20);
    for (int i = 0; i < 20; i++) {
        CHECK(v.data[i] == i);
    }

    IntVec_clear(&v);
    CHECK(IntVec_shrink_to_fit(&v) && v.data == NULL && v.cap == 0);

    IntVec_deinit(&v);
}

static void test_clear_and_deinit(void) {
    IntVec v;
    IntVec_init(&v);
    for (int i = 0; i < 20; i++) {
        IntVec_push(&v, i);
    }

    size_t cap = v.cap;
    IntVec_clear(&v);
    CHECK(v.len == 0 && v.cap == cap && v.data != NULL);

    IntVec_deinit(&v);
    CHECK(v.data == NULL && v.len == 0 && v.cap == 0);
    IntVec_deinit(&v); /* safe to call twice */
}

static void test_struct_elements(void) {
    PointVec v;
    PointVec_init(&v);

    for (int i = 0; i < 30; i++) {
        Point p = {(float)i, (float)(i * 2)};
        CHECK(PointVec_push(&v, p));
    }

    CHECK(PointVec_get(&v, 29).x == 29.0f && PointVec_get(&v, 29).y == 58.0f);
    CHECK(PointVec_last(&v)->y == 58.0f);

    PointVec_deinit(&v);
}

static void test_pointer_elements(void) {
    StrVec v;
    StrVec_init(&v);

    char *words[] = {"alpha", "beta", "gamma"};
    CHECK(StrVec_push_many(&v, words, 3));
    CHECK(StrVec_push(&v, "delta"));
    CHECK(strcmp(StrVec_get(&v, 1), "beta") == 0);
    CHECK(strcmp(*StrVec_last(&v), "delta") == 0);

    StrVec_deinit(&v);
}

/* Differential test: random operations compared against a plain array. */
static unsigned long rng_state = 12345;

static unsigned rng(void) {
    rng_state = rng_state * 6364136223846793005UL + 1442695040888963407UL;
    return (unsigned)(rng_state >> 33);
}

static void test_random_against_model(void) {
    enum {
        MAX = 4000
    };
    static int model[MAX + 1];
    size_t mlen = 0;

    IntVec v;
    IntVec_init(&v);

    for (int step = 0; step < 50000; step++) {
        int val = (int)rng();
        unsigned op = rng() % 10;

        if (op <= 2) {
            if (mlen < MAX) {
                CHECK(IntVec_push(&v, val));
                model[mlen++] = val;
            }
        } else if (op == 3) {
            int out = 0;
            bool ok = IntVec_pop(&v, &out);
            CHECK(ok == (mlen > 0));
            if (ok) {
                CHECK(out == model[--mlen]);
            }
        } else if (op == 4) {
            if (mlen < MAX) {
                size_t i = rng() % (mlen + 1);
                CHECK(IntVec_insert(&v, i, val));
                memmove(&model[i + 1], &model[i], (mlen - i) * sizeof(int));
                model[i] = val;
                mlen++;
            }
        } else if (op == 5) {
            size_t i = rng() % (mlen + 2); /* sometimes out of range */
            bool ok = IntVec_remove(&v, i);
            CHECK(ok == (i < mlen));
            if (ok) {
                memmove(&model[i], &model[i + 1], (mlen - i - 1) * sizeof(int));
                mlen--;
            }
        } else if (op == 6) {
            size_t i = rng() % (mlen + 2);
            bool ok = IntVec_swap_remove(&v, i);
            CHECK(ok == (i < mlen));
            if (ok) {
                model[i] = model[mlen - 1];
                mlen--;
            }
        } else if (op == 7) {
            CHECK(IntVec_reserve(&v, rng() % 5000));
        } else if (op == 8 && rng() % 20 == 0) {
            CHECK(IntVec_shrink_to_fit(&v));
        } else if (op == 9 && rng() % 200 == 0) {
            IntVec_clear(&v);
            mlen = 0;
        }

        CHECK(v.len == mlen);
        if (mlen > 0) {
            CHECK(memcmp(v.data, model, mlen * sizeof(int)) == 0);
        }
    }

    IntVec_deinit(&v);
}

int main(void) {
    test_init_and_empty();
    test_push_and_growth();
    test_push_own_element();
    test_pop();
    test_access();
    test_insert();
    test_remove();
    test_swap_remove();
    test_push_many();
    test_reserve();
    test_shrink_to_fit();
    test_clear_and_deinit();
    test_struct_elements();
    test_pointer_elements();
    test_random_against_model();

    printf("%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
