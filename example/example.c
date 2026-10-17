#include "../my_vector.h"

#include <stdio.h>

// create the vector implementation
VEC_DEFINE(int, IntVec)

// the macro doesn't provide a print function so
// i created my own
void IntVec_println(IntVec *v) {
    for (size_t i = 0; i < v->len; ++i) {
        printf("%d\n", v->data[i]);
    }
    printf("\n");
}

int main(void) {
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

    return 0;
}
