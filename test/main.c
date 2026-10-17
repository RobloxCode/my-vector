#include "../my_vector.h"

#include <stdio.h>

VEC_DEFINE(int, nums)

int main(void) {
    nums *n;
    nums_init(n);

    nums_free(n);

    return 0;
}
