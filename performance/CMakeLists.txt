# Test executables

#
# This function adds a Sequential test
#
function (add_seq_krowkee_benchmark bench_name)
    set(bench_source "${bench_name}.cpp")
    set(bench_exe "${bench_name}")
    add_executable(${bench_exe} ${bench_source})
    target_link_libraries(${bench_exe} PRIVATE krowkee)
    target_include_directories(
        ${bench_exe}
        PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
    )
endfunction ()

add_seq_krowkee_benchmark(histogram)
add_seq_krowkee_benchmark(init)
# add_seq_krowkee_benchmark(graph)
