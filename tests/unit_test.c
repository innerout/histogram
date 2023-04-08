#include <histogram.h>
#include <stdio.h>

int main()
{
    Histogram histogram;
    initialize_histogram(&histogram);
    printf("Buckets = %lu\n", atomic_load_explicit(&histogram.num_buckets, memory_order_relaxed));
    return 0;
}