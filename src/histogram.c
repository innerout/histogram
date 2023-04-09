#include <histogram.h>
#include <stddef.h>
#include <assert.h>
#include <math.h>
#include <limits.h>

static size_t lower_bound(const uint64_t buckets[], const uint64_t size, const uint64_t value);
static size_t histogram_get_bucket_index(const Histogram *histogram, uint64_t value);
static void initialize_bucket_map(Histogram *histogram)
{
    histogram->bucket_map.bucket_value[0] = 1;
    histogram->bucket_map.bucket_value[1] = 2;
    size_t num_elements = 1;

    double bucket_val = (double)histogram->bucket_map.bucket_value[num_elements];
    while ((bucket_val = 1.5 * bucket_val) <= (double)UINT64_MAX)
    {
        histogram->bucket_map.bucket_value[++num_elements] = (uint64_t)bucket_val;
        uint64_t pow_of_ten = 1;
        while (histogram->bucket_map.bucket_value[num_elements] / 10 > 10)
        {
            histogram->bucket_map.bucket_value[num_elements] /= 10;
            pow_of_ten *= 10;
        }
        histogram->bucket_map.bucket_value[num_elements] *= pow_of_ten;
    }
    histogram->bucket_map.maxBucketValue = histogram->bucket_map.bucket_value[num_elements];
    histogram->bucket_map.minBucketValue = histogram->bucket_map.bucket_value[0];
}

void initialize_histogram(Histogram *histogram)
{
    initialize_bucket_map(histogram);
    *(uint64_t *)&histogram->num_buckets = BUCKETSIZE;
    atomic_init(&histogram->min, UINT64_MAX);
    atomic_init(&histogram->max, 0);
    atomic_init(&histogram->num, 0);
    atomic_init(&histogram->sum, 0);
    atomic_init(&histogram->sum_squares, 0);
    for (size_t i = 0; i < histogram->num_buckets; ++i)
        atomic_init(&histogram->buckets[i], 0);
}

void clear_histogram(Histogram *histogram)
{
    initialize_histogram(histogram);
}

void add_value_histogram(Histogram *histogram, uint64_t value)
{
    atomic_fetch_add_explicit(&histogram->buckets[histogram_get_bucket_index(histogram, value)], 1, memory_order_relaxed);
    uint64_t min = atomic_load_explicit(&histogram->min, memory_order_relaxed);
    while (value < min && !atomic_compare_exchange_weak_explicit(&histogram->min, &min, value, memory_order_relaxed, memory_order_relaxed))
        ;

    uint64_t max = atomic_load_explicit(&histogram->max, memory_order_relaxed);
    while (value > max && !atomic_compare_exchange_weak_explicit(&histogram->max, &max, value, memory_order_relaxed, memory_order_relaxed))
        ;
    atomic_fetch_add_explicit(&histogram->num, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&histogram->sum, value, memory_order_relaxed);
    atomic_fetch_add_explicit(&histogram->sum_squares, value * value, memory_order_relaxed);
}

uint64_t bucket_at(const Histogram *histogram, size_t index)
{
    return atomic_load_explicit(&histogram->buckets[index], memory_order_relaxed);
}

uint64_t min(const Histogram *histogram)
{
    return atomic_load_explicit(&histogram->min, memory_order_relaxed);
}

uint64_t max(const Histogram *histogram)
{
    return atomic_load_explicit(&histogram->max, memory_order_relaxed);
}

uint64_t num(const Histogram *histogram)
{
    return atomic_load_explicit(&histogram->num, memory_order_relaxed);
}

uint64_t sum(const Histogram *histogram)
{
    return atomic_load_explicit(&histogram->sum, memory_order_relaxed);
}

uint64_t sum_squares(const Histogram *histogram)
{
    return atomic_load_explicit(&histogram->sum_squares, memory_order_relaxed);
}

uint64_t bucket_limit(const Histogram *histogram, const size_t index)
{
    assert(index < histogram->num_buckets);
    return histogram->bucket_map.bucket_value[index];
}

double median_histogram(Histogram *histogram) { return percentile_histogram(histogram, 50.0); }

double average_histogram(Histogram *histogram)
{
    uint64_t cur_num = num(histogram);
    if (cur_num == 0)
        return 0;

    uint64_t cur_sum = sum(histogram);
    return (double)cur_sum / (double)cur_num;
}

double standard_deviation_histogram(Histogram *histogram)
{
    double cur_num =
        (double)num(histogram); // Use double to avoid integer overflow
    if (cur_num == 0.0)
        return 0.0;
    double cur_sum = (double)(sum(histogram));
    double cur_sum_squares = (double)(sum_squares(histogram));

    double variance =
        (cur_sum_squares * cur_num - cur_sum * cur_sum) / (cur_num * cur_num);

    return sqrt(variance > 0.0 ? variance : 0.0);
}

double percentile_histogram(Histogram *histogram, double p)
{
    double threshold = num(histogram) * (p / 100.0);
    uint64_t cumulative_sum = 0;
    for (unsigned int b = 0; b < histogram->num_buckets; b++)
    {
        uint64_t bucket_value = bucket_at(histogram, b);
        cumulative_sum += bucket_value;
        if (cumulative_sum >= threshold)
        {
            // Scale linearly within this bucket
            uint64_t left_point = (b == 0) ? 0 : bucket_limit(histogram, b - 1);
            uint64_t right_point = bucket_limit(histogram, b);
            uint64_t left_sum = cumulative_sum - bucket_value;
            uint64_t right_sum = cumulative_sum;
            double pos = 0;
            uint64_t right_left_diff = right_sum - left_sum;
            if (right_left_diff != 0)
            {
                pos = (threshold - left_sum) / right_left_diff;
            }
            double r = left_point + (right_point - left_point) * pos;
            uint64_t cur_min = min(histogram);
            uint64_t cur_max = max(histogram);
            if (r < cur_min)
                r = (double)cur_min;
            if (r > cur_max)
                r = (double)cur_max;

            return r;
        }
    }
    return (double)max(histogram);
}

static size_t histogram_get_bucket_index(const Histogram *histogram, uint64_t value)
{
    if (value > histogram->bucket_map.maxBucketValue)
        return BUCKETSIZE - 1;

    return lower_bound(histogram->bucket_map.bucket_value, BUCKETSIZE, value);
}

static size_t lower_bound(const uint64_t buckets[], const uint64_t size, const uint64_t value)
{
    uint64_t mid;

    uint64_t low = 0;
    uint64_t high = size;

    while (low < high)
    {
        mid = low + (high - low) / 2;

        if (value <= buckets[mid])
            high = mid;
        else
            low = mid + 1;
    }

    if (low < size && buckets[low] < value)
        ++low;

    return low;
}
