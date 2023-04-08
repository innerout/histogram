#include <stdint.h>
#include <stdatomic.h>
#define BUCKETSIZE 109
typedef struct bucket_mapper
{
    uint64_t bucket_value[BUCKETSIZE];
    uint64_t maxBucketValue;
    uint64_t minBucketValue;
} bucket_mapper;

typedef struct Histogram
{
    atomic_uint_fast64_t buckets[BUCKETSIZE];
    uint64_t num_buckets;
    atomic_uint_fast64_t min;
    atomic_uint_fast64_t max;
    atomic_uint_fast64_t num;
    atomic_uint_fast64_t sum;
    atomic_uint_fast64_t sum_squares;
    bucket_mapper bucket_map;
} Histogram;

void initialize_histogram(Histogram *histogram);
void clear_histogram(Histogram *histogram);
void add_histogram(Histogram *histogram, uint64_t value);
double median_histogram(Histogram *histogram);