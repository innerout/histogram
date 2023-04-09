# Histogram
A simple lock-free histogram library implemented in C11
This code is a port of RocksDB's histogram class to C.

## Usage

You can include the code below in your `cmake` project and `#include <histogram.h>` in your sources.

``` cmake
FetchContent_Declare(histogram # Recommendation: Stick close to the original name.
                     GIT_REPOSITORY https://github.com/innerout/histogram.git)
FetchContent_GetProperties(histogram)
if(NOT histogram_POPULATED)
  FetchContent_Populate(histogram)
  add_subdirectory(${histogram_SOURCE_DIR} ${histogram_BINARY_DIR})
  FetchContent_MakeAvailable(histogram)
endif()
target_link_libraries(YOUR_LIB_NAME histogram)
```

The library provides 7 functions:

```c
void initialize_histogram(Histogram *histogram);
void clear_histogram(Histogram *histogram);
void add_value_histogram(Histogram *histogram, uint64_t value);
double median_histogram(Histogram *histogram);
double average_histogram(Histogram *histogram);
double standard_deviation_histogram(Histogram *histogram);
double percentile_histogram(Histogram *histogram, double p);
```
Check [example.c](./tests/example.c) on how to use the library.
