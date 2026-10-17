#include "../my_vector.h"

#include <stdio.h>

// create the vector implementations
VEC_DEFINE(int, IntVec)

typedef struct {
    float x, y;
} Point;

VEC_DEFINE(Point, PointVec)

// pointer types work directly, no typedef needed
VEC_DEFINE(char *, StrVec)

// the macro doesn't provide print functions so
// i created my own
void IntVec_println(IntVec *v) {
    for (size_t i = 0; i < v->len; ++i) {
        printf("%d\n", v->data[i]);
    }
    printf("\n");
}

void IntVec_print_inline(const IntVec *v) {
    printf("[");
    for (size_t i = 0; i < v->len; ++i) {
        printf(i == 0 ? "%d" : ", %d", v->data[i]);
    }
    printf("]  (len %zu, cap %zu)\n", v->len, v->cap);
}

// every function that allocates can fail, so this is a quick way to stop
// the example if that ever happens
static void must(bool ok) {
    if (!ok) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
}

// the vector doesn't own what the strings point to, so strings that are
// malloc'd have to be freed by us
static char *copy_str(const char *s) {
    size_t n = strlen(s) + 1;
    char *copy = malloc(n);
    must(copy != NULL);
    memcpy(copy, s, n);
    return copy;
}

static void demo_basic(void) {
    printf("== basic ==\n");

    // creating the vector
    IntVec nums;

    // using the functions
    IntVec_init(&nums);

    IntVec_push(&nums, 1);
    IntVec_push(&nums, 2);
    IntVec_push(&nums, 3);

    IntVec_insert(&nums, 0, 0);

    IntVec_println(&nums);

    IntVec_remove(&nums, 0);

    IntVec_println(&nums);

    printf("num at 0: %d\n", *IntVec_at(&nums, 0));

    IntVec_deinit(&nums);
}

static void demo_access_and_removal(void) {
    printf("== access and removal ==\n");

    IntVec v;
    IntVec_init(&v);

    // push_many copies a whole array in one go
    int initial[] = {10, 20, 30, 40, 50};
    must(IntVec_push_many(&v, initial, 5));
    IntVec_print_inline(&v);

    // get returns the value, asserts if the index is out of bounds
    printf("get(2) = %d\n", IntVec_get(&v, 2));

    // at returns a pointer, or NULL when out of bounds, so it is safe to probe
    int *p = IntVec_at(&v, 99);
    printf("at(99) is %s\n", p == NULL ? "NULL" : "valid");

    // last returns a pointer to the final element (NULL if empty)
    printf("last = %d\n", *IntVec_last(&v));

    // remove keeps the order, but shifts everything after it
    IntVec_remove(&v, 1);
    IntVec_print_inline(&v);

    // swap_remove is O(1) but moves the last element into the hole
    IntVec_swap_remove(&v, 0);
    IntVec_print_inline(&v);

    // pop removes the last element, optionally handing it back
    int popped;
    if (IntVec_pop(&v, &popped)) {
        printf("popped %d\n", popped);
    }
    IntVec_print_inline(&v);

    IntVec_deinit(&v);
}

static void demo_capacity(void) {
    printf("== capacity ==\n");

    IntVec v;
    IntVec_init(&v);
    printf("fresh:         len %zu, cap %zu\n", v.len, v.cap);

    // reserve ahead of time to avoid repeated reallocation
    must(IntVec_reserve(&v, 100));
    printf("after reserve: len %zu, cap %zu\n", v.len, v.cap);

    for (int i = 0; i < 10; i++) {
        must(IntVec_push(&v, i));
    }
    printf("after 10 push: len %zu, cap %zu\n", v.len, v.cap);

    // shrink_to_fit gives the unused memory back
    must(IntVec_shrink_to_fit(&v));
    printf("after shrink:  len %zu, cap %zu\n", v.len, v.cap);

    // clear resets len but keeps the buffer for reuse
    IntVec_clear(&v);
    printf("after clear:   len %zu, cap %zu\n", v.len, v.cap);

    IntVec_deinit(&v);
}

static void demo_structs(void) {
    printf("== structs ==\n");

    PointVec points;
    PointVec_init(&points);

    for (int i = 0; i < 4; i++) {
        Point p = {(float)i, (float)(i * i)};
        must(PointVec_push(&points, p));
    }

    // at gives a pointer, so elements can be modified in place
    PointVec_at(&points, 1)->x = 42.0f;

    for (size_t i = 0; i < points.len; i++) {
        Point p = PointVec_get(&points, i);
        printf("(%.1f, %.1f)\n", p.x, p.y);
    }

    PointVec_deinit(&points);
}

static void demo_strings(void) {
    printf("== strings ==\n");

    StrVec words;
    StrVec_init(&words);

    must(StrVec_push(&words, copy_str("hello")));
    must(StrVec_push(&words, copy_str("vector")));
    must(StrVec_push(&words, copy_str("world")));

    for (size_t i = 0; i < words.len; i++) {
        printf("%s\n", StrVec_get(&words, i));
    }

    // the vector only frees its own buffer, so free every string first
    for (size_t i = 0; i < words.len; i++) {
        free(words.data[i]);
    }
    StrVec_deinit(&words);
}

int main(void) {
    demo_basic();
    demo_access_and_removal();
    demo_capacity();
    demo_structs();
    demo_strings();

    return 0;
}
