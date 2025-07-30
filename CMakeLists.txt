cmake_minimum_required(VERSION 3.14)

# ---------------------------------------------------------------------------- #
# CMake policy
# ---------------------------------------------------------------------------- #
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.30")
    cmake_policy(SET CMP0167 NEW)
endif ()

# ---------------------------------------------------------------------------- #
# krowkee configuration
# ---------------------------------------------------------------------------- #
project(
    krowkee
    VERSION 0.1
    DESCRIPTION "HPC multi-stream sketching library"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#
# Configuration options for krowkee.
#
option(KROWKEE_BUILD_TESTS "Build tests" OFF)
option(KROWKEE_BUILD_EXAMPLES "Build examples" OFF)
option(KROWKEE_USE_BOOST "Compile using boost::container::flat_map" ON)
option(KROWKEE_USE_YGM "Compile using YGM features (only for testing)" OFF)
option(KROWKEE_DOXYGEN "Adds targets for generating Doxygen documentation" OFF)
option(KROWKEE_RTD_ONLY
       "Run Cmake for only generating documentation for Read the Docs" OFF
)

#
# Doxygen and Read the Docs (RTD) Documentation
#
if (KROWKEE_DOXYGEN OR KROWKEE_RTD_ONLY)
    add_subdirectory(docs)
    if (KROWKEE_RTD_ONLY)
        return()
    endif ()
endif ()

# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if (EXISTS "${LOC_PATH}")
    message(
        FATAL_ERROR "Cannot build in a directory with a CMakeLists.txt file. "
                    "Please make a build subdirectory."
    )
endif ()

# Prepare to invoke FetchContent
include(FetchContent)

if (NOT boost_POPULATED)
    find_package(Boost 1.78)
    if (Boost_FOUND)
        message(STATUS ${PROJECT_NAME} " found boost include dirs: "
                       ${Boost_INCLUDE_DIRS}
        )
    else ()
        FetchContent_Declare(
            Boost
            URL https://github.com/boostorg/boost/releases/download/boost-1.87.0/boost-1.87.0-cmake.tar.gz
        )
        set(BOOST_INCLUDE_LIBRARIES container)
        FetchContent_MakeAvailable(Boost)
    endif ()
endif ()

if (KROWKEE_USE_YGM)
    #
    # Load YGM
    #
    if (NOT ygm_POPULATED)
        find_package(ygm CONFIG QUIET)
        if (NOT ygm_FOUND)
            set(JUST_INSTALL_YGM TRUE)
            FetchContent_Declare(
                ygm
                GIT_REPOSITORY https://github.com/LLNL/ygm.git
                GIT_TAG v0.8-dev
            )
            FetchContent_MakeAvailable(ygm)
        endif ()
    endif ()
else ()
    message(STATUS ${PROJECT_NAME} " building without ygm")
endif ()

add_library(krowkee INTERFACE)
add_library(krowkee::krowkee ALIAS krowkee)
target_include_directories(
    krowkee INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                      $<INSTALL_INTERFACE:include>
)
target_compile_features(krowkee INTERFACE cxx_std_20)
if (KROWKEE_USE_YGM)
    target_link_libraries(krowkee INTERFACE ygm::ygm)
endif ()
if (Boost_FOUND)
    target_include_directories(krowkee INTERFACE ${Boost_INCLUDE_DIRS})
endif ()

if (KROWKEE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif ()

if (KROWKEE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

if (KROWKEE_BUILD_PERFORMANCE)
    add_subdirectory(performance)
endif ()
