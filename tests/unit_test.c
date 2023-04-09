#include <histogram.h>
#include <stdio.h>

int main()
{
    Histogram histogram;
    initialize_histogram(&histogram);
    for (size_t i = 0; i < 10000000; i++)
    {
        add_value_histogram(&histogram, i);
    }
    printf("Average = %f\n", average_histogram(&histogram));
    printf("Median = %f\n", median_histogram(&histogram));
    printf("Standard Deviation = %f\n", standard_deviation_histogram(&histogram));
    printf("Percentile 99.999 = %f\n", percentile_histogram(&histogram, 99.999));
    printf("Percentile 99.99 = %f\n", percentile_histogram(&histogram, 99.99));
    printf("Percentile 99.9 = %f\n", percentile_histogram(&histogram, 99.9));
    printf("Percentile 99 = %f\n", percentile_histogram(&histogram, 99));
    printf("Percentile 95 = %f\n", percentile_histogram(&histogram, 95));
    printf("Percentile 90 = %f\n", percentile_histogram(&histogram, 90));
    printf("Percentile 75 = %f\n", percentile_histogram(&histogram, 75));
    printf("Percentile 50 = %f\n", percentile_histogram(&histogram, 50));
    printf("Percentile 25 = %f\n", percentile_histogram(&histogram, 25));
    printf("Percentile 10 = %f\n", percentile_histogram(&histogram, 10));
    printf("Percentile 5 = %f\n", percentile_histogram(&histogram, 5));
    printf("Percentile 1 = %f\n", percentile_histogram(&histogram, 1));

    return 0;
}