#include <histogram.h>
#include <stddef.h>
#include <assert.h>
#include <math.h>

static size_t lower_bound(const atomic_uint_fast64_t buckets[], const uint64_t size, const uint64_t value);
static size_t histogram_get_bucket_index(const Histogram *histogram, uint64_t value);
static double percentile_histogram(Histogram *histogram, double p);

// This function initializes a histogram to have all 0's in it.
void initialize_histogram(Histogram *histogram)
{
    atomic_init(&histogram->min, UINT64_MAX);
    atomic_init(&histogram->max, 0);
    atomic_init(&histogram->num, 0);
    atomic_init(&histogram->sum, 0);
    atomic_init(&histogram->sum_squares, 0);
    for (size_t i = 0; i < histogram->num_buckets; ++i)
        atomic_init(&histogram->buckets[i], 0);
}

/* Clears the histogram, setting all values to zero. */
void clear_histogram(Histogram *histogram)
{
    initialize_histogram(histogram);
}

/* A histogram is a collection of counters that are arranged in bins. This
 * function adds the given value to the appropriate bin. If the value is
 * too large to fit in any bin, it is added to the overflow bin. */
void add_histogram(Histogram *histogram, uint64_t value)
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

// Returns the upper bound of the bucket at the given index. If the index is
// equal to the number of buckets, then the return value is the upper bound of
// the overflow bucket.
uint64_t bucket_at(const Histogram *histogram, size_t index)
{
    return atomic_load_explicit(&histogram->buckets[index], memory_order_relaxed);
}

// This function computes the minimum value of the histogram
// (the histogram's "minimum value" is the smallest value that
// the histogram contains).
uint64_t min(const Histogram *histogram)
{
    return atomic_load_explicit(&histogram->min, memory_order_relaxed);
}

/**
 * @brief Get the maximum value in a histogram
 * @param histogram The histogram
 * @return The maximum value
 */
uint64_t max(const Histogram *histogram)
{
    return atomic_load_explicit(&histogram->max, memory_order_relaxed);
}

/* Returns the number of elements in the histogram. */
uint64_t num(const Histogram *histogram)
{
    return atomic_load_explicit(&histogram->num, memory_order_relaxed);
}

// Computes the sum of the elements in a histogram
// histogram - Pointer to the histogram
uint64_t sum(const Histogram *histogram)
{
    return atomic_load_explicit(&histogram->sum, memory_order_relaxed);
}

// Computes the sum of the squares of the histogram elements.
// histogram: Histogram for which to compute the sum of squares.
uint64_t sum_squares(const Histogram *histogram)
{
    return atomic_load_explicit(&histogram->sum_squares, memory_order_relaxed);
}

// This function returns the bucket limit for a given index in the histogram.
// The bucket limit is the upper bound of the bucket, so for example if the
// bucket limits are [0, 10, 20, 30] then the bucket limit of index 2 is 20. The
// bucket limits are stored in the histogram as an array of uint64_t values, one
// for each bucket.
uint64_t bucket_limit(const Histogram *histogram, const size_t index)
{
    assert(index < histogram->num_buckets);
    return histogram->bucket_map.bucket_value[index];
}

/* Returns the median value of a histogram. */
double median_histogram(Histogram *histogram) { return percentile_histogram(histogram, 50.0); }

/* Returns the average value of a histogram. */
double average_histogram(Histogram *histogram)
{
    uint64_t cur_num = num(histogram);
    if (cur_num == 0)
        return 0;

    uint64_t cur_sum = sum(histogram);
    return (double)cur_sum / (double)cur_num;
}

/* Returns the standard deviation value of a histogram. */
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

// This function computes the pth percentile of a histogram.  The histogram is
// an array of the number of points in each bin.  The number of points in each
// bin is proportional to the density of points in that bin.  The function
// returns the value of the data in the bin that contains the pth percentile of
// the data. The variable p is a number between 0 and 1.  The function returns
// the value of the data in the bin that contains the p*Nth data point. The
// variable histogram->bin_size contains the width of each bin. The variable
// histogram->min contains the minimum value of the data that was histogrammed.
static double percentile_histogram(Histogram *histogram, double p)
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

// Needs fixing
static size_t histogram_get_bucket_index(const Histogram *histogram, uint64_t value)
{
    return lower_bound(histogram->buckets, histogram->num_buckets, value);
}

static size_t lower_bound(const atomic_uint_fast64_t buckets[], const uint64_t size, const uint64_t value)
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
