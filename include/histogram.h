#include <stdint.h>
#include <stdatomic.h>
typedef struct Histogram
{
    atomic_uint_fast64_t min;
    atomic_uint_fast64_t max;
    atomic_uint_fast64_t num;
    atomic_uint_fast64_t sum;
    atomic_uint_fast64_t sum_squares;
    atomic_uint_fast64_t buckets[109];
    const uint64_t num_buckets;
} Histogram;
