#include <histogram.h>

uint64_t lower_bound(const atomic_uint_fast64_t buckets[], uint64_t size, uint64_t value)
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
