# Test executables

#
# This function adds a Sequential test
#
function (add_seq_krowkee_benchmark bench_name)
    set(test_source "${bench_name}.cpp")
    set(test_exe "${bench_name}")
    add_executable(${test_exe} ${test_source})
    target_link_libraries(${test_exe} PRIVATE krowkee)
    target_include_directories(
        ${test_exe}
        PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
    )
    add_test(${test_exe} "${CMAKE_CURRENT_BINARY_DIR}/${test_exe}")
endfunction ()

add_seq_krowkee_benchmark(vs_histogram)
