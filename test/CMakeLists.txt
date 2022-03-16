# Test executables

#
# This function adds a Sequential test
#
function (add_seq_krowkee_test test_name)
    set(test_source "${test_name}.cpp")
    set(test_exe "SEQ_${test_name}")
    add_executable(${test_exe} ${test_source})
    target_link_libraries(${test_exe} PRIVATE krowkee)
    target_include_directories(
        ${test_exe}
        PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
    )
    add_test(${test_exe} "${CMAKE_CURRENT_BINARY_DIR}/${test_exe}")
endfunction ()

add_seq_krowkee_test(hash_test)
add_seq_krowkee_test(test_fwht_utils)
add_seq_krowkee_test(compacting_map_test)
add_seq_krowkee_test(local_linearsketch_test)
add_seq_krowkee_test(multisketch_test)

if (KROWKEE_USE_YGM)

    add_seq_krowkee_test(communicable_linearsketch_test)

    find_package(MPI REQUIRED)

    #
    # This function adds an MPI test
    #
    function (add_mpi_krowkee_test test_name)
        set(test_source "${test_name}.cpp")
        set(test_exe "MPI_${test_name}")
        add_executable(${test_exe} ${test_source})
        target_link_libraries(${test_exe} PRIVATE krowkee)
        target_include_directories(
            ${test_exe}
            PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                    $<INSTALL_INTERFACE:include>
        )
        if (TEST_WITH_SLURM)
            add_test(${test_exe} "srun"
                     "${CMAKE_CURRENT_BINARY_DIR}/${test_exe}"
            )
        else ()
            add_test(${test_exe} ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "4"
                     ${MPIEXEC_PREFLAGS}
                     "${CMAKE_CURRENT_BINARY_DIR}/${test_exe}"
            )
        endif ()
    endfunction ()

    add_mpi_krowkee_test(distributed_test)
endif ()
