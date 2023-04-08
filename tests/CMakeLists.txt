include_directories(../include)
add_executable(unit_test unit_test.c)
target_link_libraries(unit_test histogram)
add_test(unit_test unit_test)