#include "../my_vector.h"

#include <stdio.h>

VEC_DEFINE(int, IntVec);

int main(void) {
    IntVec nums;
    IntVec_init(&nums);

    IntVec_push(&nums, 1);
    IntVec_push(&nums, 2);
    IntVec_push(&nums, 3);

    for (size_t i = 0; i < nums.len; ++i) {
        printf("%d\n", nums.data[i]);
    }

    IntVec_insert(&nums, 0, 0);
    for (size_t i = 0; i < nums.len; ++i) {
        printf("%d\n", nums.data[i]);
    }

    IntVec_remove(&nums, 0);

    for (size_t i = 0; i < nums.len; ++i) {
        printf("%d\n", nums.data[i]);
    }

    printf("num at 0: %d\n", *IntVec_at(&nums, 0));

    IntVec_free(&nums);
    return 0;
}
