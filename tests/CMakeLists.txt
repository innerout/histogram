include_directories(../include)
add_executable(example example.c)
target_link_libraries(example histogram)
add_test(example example)
