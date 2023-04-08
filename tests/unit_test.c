#include <histogram.h>
#include <stdio.h>

int main()
{
    Histogram histogram = {0};
    printf("Buckets = %lu\n", atomic_load_explicit(&histogram.num_buckets, memory_order_relaxed));
    return 0;
}