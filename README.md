# KROWKEE: A Distributed Multi-Stream Data Sketching Toolkit.

This repository implements `krowkee`, a toolkit for scalably and efficiently 
summarizing many data streams in distributed memory.
`krowkee` is intended for applications involving where one needs to summarize 
huge loosely structured data, such as matrices or graphs, where individual 
components such as rows/columns or vertex adjacency information are impractical 
to store and directly inspect.
`krowkee` ingests these objects as data streams - unstructured, arbitrarily 
ordered lists of updates - and accumulates summaries thereof in the form of data 
sketches.

Although there are many types of data sketches, the practical varieties encompassed by
krowkee have many advantages compared to directly observing data:
* approximation guarantees on some stream statistic
* merge operator
* worst-case logarithmic memory usage
* ... and many even support sparse storage

`krowkee` currently supports only 
[sparse subspace embeddings](https://arxiv.org/abs/1207.6365)
to perform randomized and fast dimensionality reduction. 
Future sketch support is planned, principally including cardinality sketches 
such as the
[HyperLogLog](https://en.wikipedia.org/wiki/HyperLogLog).

# Getting started

## Reqirements
* C++17 - GCC versions 10-11 are tested. 
Your mileage may vary with other compilers.
* Optional dependencies:
    - [YGM](https://github.com/LLNL/ygm) 0.7 or greater for distributed memory
      communication.
      Toggle with CMake option `KROWKEE_USE_YGM`. 
      Default `OFF`.
      YGM is not used within the library, but is used in some tests and examples
      that are only compiled if `KROWKEE_USE_YGM` is `ON`.
      If package `ygm` is not installed, `krowkee` will attempt to clone and
      install via 
      [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html). Includes additional dependencies:
        * [Cereal](https://github.com/USCiLab/cereal) - C++ serialization 
          library. 
          If package `cereal` is not installed, `krowkee` will attempt
          [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html).
        * MPI         
    - [Boost](https://www.boost.org/) 1.48 or greater for 
      `boost::container::flatmap`. 
      Toggle with CMake option `KROWKEE_USE_BOOST`.
      Default `ON`.

## Using `krowkee` with CMake

`krowkee` is a header-only library that is simple to incorporate into dependent
projects using CMake.
Add the following to your `CMakeLists.txt` to "find-else-fetch" `krowkee`, 
cloning it and its dependencies and preparing their headers for installation as
a part of your project.
```
set(DESIRED_KROWKEE_VERSION 0.1)
find_package(krowkee ${DESIRED_KROWKEE_VERSION} CONFIG)
if (NOT krowkee_FOUND)
    FetchContent_Declare(
        krowkee
        GIT_REPOSITORY https://github.com/LLNL/krowkee
        GIT_TAG v${DESIRED_KROWKEE_VERSION}
    )
    FetchContent_GetProperties(krowkee)
    if (krowkee_POPULATED)
        message(STATUS "Found already populated krowkee dependency: "
                       ${krowkee_SOURCE_DIR}
        )
    else ()
        set(JUST_INSTALL_KROWKEE ON)
        set(KROWKEE_INSTALL ON)
        set(KROWKEE_USE_YGM ON)  # or OFF if local-only
        FetchContent_Populate(krowkee)
        add_subdirectory(${krowkee_SOURCE_DIR} ${krowkee_BINARY_DIR})
        message(STATUS "Cloned krowkee dependency " ${krowkee_SOURCE_DIR})
    endif ()
else ()
    message(STATUS "Found installed krowkee dependency " ${krowkee_DIR})
endif ()
```


## Building
These instructions assume that you have a relatively modern C++ compiler 
(C++17 required, only tested using GCC).
If included, `krowkee`'s CMake build makes use of find-else-fetch semantics for 
its optional `ygm` and `cereal` dependencies.
`krowkee` will try to find local installations of the libraries, and will clone
and link the repositories internally if none are found.

One can build a local-only version of `krowkee` by passing 
 to `cmake .. -DKROWKEE_USE_YGM=OFF`.

`spack` is a convenient means to include `cereal` and manage compilers, but 
is not required to build `krowkee`. 

### Build steps
Clone the project and make the build directory
``` bash
$ git clone ssh://git@czgitlab.llnl.gov:7999/krowkee/krowkee.git
$ mkdir krowkee/build
$ cd krowkee/build
```

Option 1: use spack
``` bash
$ spack load gcc     # tested at >=8.3.1.
$ spack load boost   # optional, at least version 1.75
$ spack load cereal  # optional, at least version 1.3.0
```

Option 2: use module
``` bash
$ module load gcc/8.3.1  # or desired gcc version
```

Build `krowkee`.
``` bash
$ cmake ..
$ make
```

Alternately, we can build the local-only version of `krowkee` which will not
use `ygm`, `cereal`, or MPI.
``` bash
$ cmake .. -DKROWKEE_USE_YGM=OFF
$ make
```

### Installation

`krowkee` build system supports local installation of it and all dependent 
header-only libraries (`ygm` and `cereal`) via `make install`. 
If operating on a system without write permissions to CMake's default 
installtion locations, set the `CMAKE_INSTALL PREFIX`:

``` bash
$ export CMAKE_USER_INSTALL_PREFIX='/desired/install/path'
$ cmake .. -DCMAKE_INSTALL_PREFIX=${CMAKE_USER_INSTALL_PREFIX}
$ make
$ make install
```

It is most likely worth setting this `CMAKE_USER_INSTALL_PREFIX` and adding it
to your `CMAKE_PREFIX_PATH` in your `.profile`, `.bashrc`, or equivalent.  

## Testing

It is easy to run all test cases once krowkee is built by running
``` bash
make test
```

Alternately, one can directly run individual test cases with more options and 
verbose outputs, e.g.
``` bash
$ ./test/SEQ_local_linearsketch_test
```

All tests support an `-h` flag listing options.

# About

## Authors

* Benjamin W. Priest (priest2 at llnl dot gov)
* Alec Dunton (dunton1 at llnl dot gov)

# License

`krowkee` is distributed under the MIT license.

All new contributions must be made under the MIT license.

See [LICENSE-MIT](LICENSE-MIT), [NOTICE](NOTICE), and [COPYRIGHT](COPYRIGHT) 
for details.

SPDX-License-Identifier: MIT

# Release

LLNL-CODE-827987