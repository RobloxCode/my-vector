/*
 * test_vec.c - tests for vec.h
 *
 * Build and run:
 *     gcc -Wall -Wextra -std=c99 -g -fsanitize=address,undefined \
 *         -o test_vec test_vec.c && ./test_vec
 *
 * The sanitizer flags are optional but recommended: they turn memory
 * bugs (leaks, out-of-bounds, use-after-free) into test failures.
 *
 * Exit code is 0 if every check passed and 1 otherwise.
 */

#include "../my_vector.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ---------- element types under test ---------- */

typedef struct {
    float x, y;
} Point;

typedef char *String;

VEC_DEFINE(int, IntVec)
VEC_DEFINE(Point, PointVec)
VEC_DEFINE(String, StringVec)
VEC_DEFINE(char, CharVec)

/* ---------- tiny test harness ---------- */

static int checks_run = 0;
static int checks_failed = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        checks_run++;                                                          \
        if (!(cond)) {                                                         \
            checks_failed++;                                                   \
            printf("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);         \
        }                                                                      \
    } while (0)

#define RUN(test)                                                              \
    do {                                                                       \
        int before = checks_failed;                                            \
        test();                                                                \
        printf("[%s] %s\n", checks_failed == before ? "PASS" : "FAIL", #test); \
    } while (0)

/* Build a vector holding 0, 1, ..., n - 1. */
static void fill_seq(IntVec *v, int n) {
    for (int i = 0; i < n; i++) {
        IntVec_push(v, i);
    }
}

/* ---------- init / free ---------- */

static void test_init(void) {
    IntVec v;
    IntVec_init(&v);
    CHECK(v.data == NULL);
    CHECK(v.len == 0);
    CHECK(v.cap == 0);
}

static void test_free_resets_state(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 5);
    IntVec_free(&v);
    CHECK(v.data == NULL);
    CHECK(v.len == 0);
    CHECK(v.cap == 0);
}

static void test_double_free_is_safe(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 5);
    IntVec_free(&v);
    IntVec_free(&v);
    CHECK(v.data == NULL);
}

static void test_free_empty_vector(void) {
    IntVec v;
    IntVec_init(&v);
    IntVec_free(&v);
    CHECK(v.data == NULL);
    CHECK(v.len == 0);
}

static void test_reuse_after_free(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 3);
    IntVec_free(&v);
    CHECK(IntVec_push(&v, 99));
    CHECK(v.len == 1);
    CHECK(v.data[0] == 99);
    IntVec_free(&v);
}

/* ---------- reserve ---------- */

static void test_reserve_initial_capacity(void) {
    IntVec v;
    IntVec_init(&v);
    CHECK(IntVec_reserve(&v, 1));
    CHECK(v.cap == VEC_INITIAL_CAP);
    CHECK(v.len == 0);
    IntVec_free(&v);
}

static void test_reserve_grows_by_doubling(void) {
    IntVec v;
    IntVec_init(&v);
    CHECK(IntVec_reserve(&v, VEC_INITIAL_CAP + 1));
    CHECK(v.cap == VEC_INITIAL_CAP * 2);
    CHECK(IntVec_reserve(&v, 100));
    CHECK(v.cap >= 100);
    IntVec_free(&v);
}

static void test_reserve_never_shrinks(void) {
    IntVec v;
    IntVec_init(&v);
    IntVec_reserve(&v, 100);
    size_t cap = v.cap;
    CHECK(IntVec_reserve(&v, 1));
    CHECK(v.cap == cap);
    IntVec_free(&v);
}

static void test_reserve_overflow_fails_cleanly(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 4);
    int *old_data = v.data;
    size_t old_cap = v.cap;

    CHECK(!IntVec_reserve(&v, SIZE_MAX));
    CHECK(v.data == old_data);
    CHECK(v.cap == old_cap);
    CHECK(v.len == 4);
    CHECK(v.data[3] == 3);
    IntVec_free(&v);
}

static void test_reserve_keeps_contents(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 5);
    CHECK(IntVec_reserve(&v, 1000));
    for (int i = 0; i < 5; i++) {
        CHECK(v.data[i] == i);
    }
    IntVec_free(&v);
}

/* ---------- push ---------- */

static void test_push_single(void) {
    IntVec v;
    IntVec_init(&v);
    CHECK(IntVec_push(&v, 42));
    CHECK(v.len == 1);
    CHECK(v.data[0] == 42);
    CHECK(v.cap == VEC_INITIAL_CAP);
    IntVec_free(&v);
}

static void test_push_many_preserves_order(void) {
    IntVec v;
    IntVec_init(&v);
    for (int i = 0; i < 10000; i++) {
        CHECK(IntVec_push(&v, i * 3));
    }
    CHECK(v.len == 10000);
    CHECK(v.cap >= v.len);
    int ok = 1;
    for (int i = 0; i < 10000; i++) {
        if (v.data[i] != i * 3) {
            ok = 0;
            break;
        }
    }
    CHECK(ok);
    IntVec_free(&v);
}

static void test_push_capacity_boundary(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, VEC_INITIAL_CAP);
    CHECK(v.cap == VEC_INITIAL_CAP);
    IntVec_push(&v, 123);
    CHECK(v.cap == VEC_INITIAL_CAP * 2);
    CHECK(v.len == VEC_INITIAL_CAP + 1);
    CHECK(v.data[VEC_INITIAL_CAP] == 123);
    IntVec_free(&v);
}

/* ---------- pop ---------- */

static void test_pop_returns_last(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 3);
    int out = -1;
    CHECK(IntVec_pop(&v, &out));
    CHECK(out == 2);
    CHECK(v.len == 2);
    CHECK(IntVec_pop(&v, &out));
    CHECK(out == 1);
    IntVec_free(&v);
}

static void test_pop_with_null_out(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 3);
    CHECK(IntVec_pop(&v, NULL));
    CHECK(v.len == 2);
    IntVec_free(&v);
}

static void test_pop_empty_fails(void) {
    IntVec v;
    IntVec_init(&v);
    int out = 7;
    CHECK(!IntVec_pop(&v, &out));
    CHECK(out == 7);
    CHECK(v.len == 0);
    IntVec_free(&v);
}

static void test_pop_until_empty(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 20);
    for (int i = 19; i >= 0; i--) {
        int out;
        CHECK(IntVec_pop(&v, &out));
        CHECK(out == i);
    }
    CHECK(v.len == 0);
    CHECK(!IntVec_pop(&v, NULL));
    IntVec_free(&v);
}

/* ---------- at ---------- */

static void test_at_valid_and_invalid(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 3);
    CHECK(IntVec_at(&v, 0) == &v.data[0]);
    CHECK(*IntVec_at(&v, 2) == 2);
    CHECK(IntVec_at(&v, 3) == NULL);
    CHECK(IntVec_at(&v, SIZE_MAX) == NULL);
    IntVec_free(&v);
}

static void test_at_allows_modification(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 3);
    *IntVec_at(&v, 1) = 500;
    CHECK(v.data[1] == 500);
    IntVec_free(&v);
}

static void test_at_on_empty(void) {
    IntVec v;
    IntVec_init(&v);
    CHECK(IntVec_at(&v, 0) == NULL);
}

/* ---------- insert ---------- */

static void test_insert_front(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 3);
    CHECK(IntVec_insert(&v, 0, 100));
    CHECK(v.len == 4);
    CHECK(v.data[0] == 100);
    CHECK(v.data[1] == 0);
    CHECK(v.data[3] == 2);
    IntVec_free(&v);
}

static void test_insert_middle(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 4);
    CHECK(IntVec_insert(&v, 2, 100));
    int expected[] = {0, 1, 100, 2, 3};
    CHECK(v.len == 5);
    CHECK(memcmp(v.data, expected, sizeof expected) == 0);
    IntVec_free(&v);
}

static void test_insert_at_end_appends(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 3);
    CHECK(IntVec_insert(&v, 3, 100));
    CHECK(v.len == 4);
    CHECK(v.data[3] == 100);
    IntVec_free(&v);
}

static void test_insert_into_empty(void) {
    IntVec v;
    IntVec_init(&v);
    CHECK(IntVec_insert(&v, 0, 5));
    CHECK(v.len == 1);
    CHECK(v.data[0] == 5);
    IntVec_free(&v);
}

static void test_insert_out_of_range_fails(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 3);
    CHECK(!IntVec_insert(&v, 4, 100));
    CHECK(!IntVec_insert(&v, SIZE_MAX, 100));
    CHECK(v.len == 3);
    CHECK(v.data[2] == 2);
    IntVec_free(&v);
}

static void test_insert_triggers_growth(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, VEC_INITIAL_CAP);
    CHECK(IntVec_insert(&v, 0, -1));
    CHECK(v.cap == VEC_INITIAL_CAP * 2);
    CHECK(v.data[0] == -1);
    CHECK(v.data[VEC_INITIAL_CAP] == VEC_INITIAL_CAP - 1);
    IntVec_free(&v);
}

/* ---------- remove ---------- */

static void test_remove_front(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 4);
    CHECK(IntVec_remove(&v, 0));
    int expected[] = {1, 2, 3};
    CHECK(v.len == 3);
    CHECK(memcmp(v.data, expected, sizeof expected) == 0);
    IntVec_free(&v);
}

static void test_remove_middle(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 5);
    CHECK(IntVec_remove(&v, 2));
    int expected[] = {0, 1, 3, 4};
    CHECK(v.len == 4);
    CHECK(memcmp(v.data, expected, sizeof expected) == 0);
    IntVec_free(&v);
}

static void test_remove_last(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 3);
    CHECK(IntVec_remove(&v, 2));
    CHECK(v.len == 2);
    CHECK(v.data[1] == 1);
    IntVec_free(&v);
}

static void test_remove_only_element(void) {
    IntVec v;
    IntVec_init(&v);
    IntVec_push(&v, 1);
    CHECK(IntVec_remove(&v, 0));
    CHECK(v.len == 0);
    IntVec_free(&v);
}

static void test_remove_out_of_range_fails(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 3);
    CHECK(!IntVec_remove(&v, 3));
    CHECK(!IntVec_remove(&v, SIZE_MAX));
    CHECK(v.len == 3);
    IntVec_free(&v);

    IntVec empty;
    IntVec_init(&empty);
    CHECK(!IntVec_remove(&empty, 0));
}

/* ---------- clear ---------- */

static void test_clear_keeps_capacity(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 20);
    int *old_data = v.data;
    size_t old_cap = v.cap;
    IntVec_clear(&v);
    CHECK(v.len == 0);
    CHECK(v.cap == old_cap);
    CHECK(v.data == old_data);
    IntVec_free(&v);
}

static void test_push_after_clear(void) {
    IntVec v;
    IntVec_init(&v);
    fill_seq(&v, 5);
    IntVec_clear(&v);
    IntVec_push(&v, 77);
    CHECK(v.len == 1);
    CHECK(v.data[0] == 77);
    IntVec_free(&v);
}

/* ---------- other element types ---------- */

static void test_struct_elements(void) {
    PointVec v;
    PointVec_init(&v);
    for (int i = 0; i < 100; i++) {
        CHECK(PointVec_push(&v, (Point){(float)i, (float)i * 2}));
    }
    CHECK(v.len == 100);
    CHECK(v.data[0].x == 0.0f);
    CHECK(v.data[99].x == 99.0f);
    CHECK(v.data[99].y == 198.0f);

    Point p;
    CHECK(PointVec_pop(&v, &p));
    CHECK(p.x == 99.0f && p.y == 198.0f);

    CHECK(PointVec_insert(&v, 1, (Point){-1.0f, -2.0f}));
    CHECK(v.data[1].x == -1.0f && v.data[1].y == -2.0f);
    CHECK(v.data[2].x == 1.0f);
    PointVec_free(&v);
}

static void test_string_pointer_elements(void) {
    StringVec v;
    StringVec_init(&v);
    StringVec_push(&v, "alpha");
    StringVec_push(&v, "beta");
    StringVec_push(&v, "gamma");
    CHECK(v.len == 3);
    CHECK(strcmp(v.data[1], "beta") == 0);
    StringVec_remove(&v, 1);
    CHECK(strcmp(v.data[1], "gamma") == 0);

    String out = NULL;
    CHECK(StringVec_pop(&v, &out));
    CHECK(strcmp(out, "gamma") == 0);
    StringVec_free(&v);
}

static void test_char_elements(void) {
    CharVec v;
    CharVec_init(&v);
    const char *msg = "hello";
    for (size_t i = 0; msg[i] != '\0'; i++) {
        CharVec_push(&v, msg[i]);
    }
    CharVec_push(&v, '\0');
    CHECK(strcmp(v.data, "hello") == 0);
    CharVec_free(&v);
}

/* ---------- independence and mixed operations ---------- */

static void test_vectors_are_independent(void) {
    IntVec a, b;
    IntVec_init(&a);
    IntVec_init(&b);
    fill_seq(&a, 5);
    IntVec_push(&b, 99);
    CHECK(a.len == 5);
    CHECK(b.len == 1);
    CHECK(a.data != b.data);
    IntVec_free(&a);
    IntVec_free(&b);
}

static void test_mixed_operations_match_model(void) {
    /* Mirror every operation on a plain array and compare at the end. */
    IntVec v;
    IntVec_init(&v);
    int model[256];
    size_t n = 0;

    for (int i = 0; i < 100; i++) {
        IntVec_push(&v, i);
        model[n++] = i;
    }
    for (int i = 0; i < 30; i++) {
        size_t idx = (size_t)(i * 7) % n;
        IntVec_remove(&v, idx);
        memmove(&model[idx], &model[idx + 1], (n - idx - 1) * sizeof(int));
        n--;
    }
    for (int i = 0; i < 30; i++) {
        size_t idx = (size_t)(i * 11) % (n + 1);
        IntVec_insert(&v, idx, 1000 + i);
        memmove(&model[idx + 1], &model[idx], (n - idx) * sizeof(int));
        model[idx] = 1000 + i;
        n++;
    }
    for (int i = 0; i < 10; i++) {
        IntVec_pop(&v, NULL);
        n--;
    }

    CHECK(v.len == n);
    CHECK(memcmp(v.data, model, n * sizeof(int)) == 0);
    IntVec_free(&v);
}

/* ---------- main ---------- */

int main(void) {
    RUN(test_init);
    RUN(test_free_resets_state);
    RUN(test_double_free_is_safe);
    RUN(test_free_empty_vector);
    RUN(test_reuse_after_free);

    RUN(test_reserve_initial_capacity);
    RUN(test_reserve_grows_by_doubling);
    RUN(test_reserve_never_shrinks);
    RUN(test_reserve_overflow_fails_cleanly);
    RUN(test_reserve_keeps_contents);

    RUN(test_push_single);
    RUN(test_push_many_preserves_order);
    RUN(test_push_capacity_boundary);

    RUN(test_pop_returns_last);
    RUN(test_pop_with_null_out);
    RUN(test_pop_empty_fails);
    RUN(test_pop_until_empty);

    RUN(test_at_valid_and_invalid);
    RUN(test_at_allows_modification);
    RUN(test_at_on_empty);

    RUN(test_insert_front);
    RUN(test_insert_middle);
    RUN(test_insert_at_end_appends);
    RUN(test_insert_into_empty);
    RUN(test_insert_out_of_range_fails);
    RUN(test_insert_triggers_growth);

    RUN(test_remove_front);
    RUN(test_remove_middle);
    RUN(test_remove_last);
    RUN(test_remove_only_element);
    RUN(test_remove_out_of_range_fails);

    RUN(test_clear_keeps_capacity);
    RUN(test_push_after_clear);

    RUN(test_struct_elements);
    RUN(test_string_pointer_elements);
    RUN(test_char_elements);

    RUN(test_vectors_are_independent);
    RUN(test_mixed_operations_match_model);

    printf("\n%d checks, %d failed\n", checks_run, checks_failed);
