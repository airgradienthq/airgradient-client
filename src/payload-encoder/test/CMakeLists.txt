# Test executable macro
macro(add_unit_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE payload_encoder unity)
    target_include_directories(${test_name} PRIVATE ../src)
    add_test(NAME ${test_name} COMMAND ${test_name})
endmacro()

# Add all test executables
add_unit_test(test_encoder test_encoder.cpp)
add_unit_test(test_single_channel test_single_channel.cpp)
add_unit_test(test_dual_channel test_dual_channel.cpp)
add_unit_test(test_batching test_batching.cpp)

# Size calculation utility (not a test)
add_executable(test_sizes test_sizes.cpp)
target_link_libraries(test_sizes PRIVATE payload_encoder)
target_include_directories(test_sizes PRIVATE ../src)

# Custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_encoder test_single_channel test_dual_channel test_batching
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
