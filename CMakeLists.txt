cmake_minimum_required(VERSION 3.14)

project(
    krowkee
    VERSION 0.1
    DESCRIPTION "HPC multi-stream sketching library"
    LANGUAGES CXX
)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(KROWKEE_MAIN_PROJECT ON)
endif ()

# Controls whether to set up install boilerplate for project and depencencies.
# Expects CMAKE_INSTALL_PREFIX to be set to a suitable directory.
option(JUST_INSTALL_KROWKEE "Skip executable compilation" OFF)
option(KROWKEE_INSTALL "Generate the install target" ${KROWKEE_MAIN_PROJECT})
option(KROWKEE_USE_BOOST "Compile using boost::container::flat_map" ON)
option(KROWKEE_USE_CEREAL "Compile using CEREAL memory features" ON)
option(KROWKEE_USE_YGM "Compile using YGM features (only for testing)" OFF)
option(KROWKEE_BUILD_DOCS "Build documentation with Doxygen and SPHINX" OFF)

if (KROWKEE_MAIN_PROJECT)

    # Let's nicely support folders in IDEs
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    # Testing only available if this is the main app NOTE: this needs to be done
    # in the main CMakeLists because it calls enable_testing, which must be in
    # the main CMakeLists
    include(CTest)

    # # Docs only available if this is the main app find_package(Doxygen) if
    # (Doxygen_FOUND) # add_subdirectory(docs) message(STATUS "Doxygen found,
    # skipping docs") else () message(STATUS "Doxygen not found, not building
    # docs") endif ()
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

endif ()

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
    cmake_policy(VERSION 3.30)
    cmake_policy(SET CMP0167 NEW)
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

if (KROWKEE_USE_CEREAL)
    #
    # Load cereal
    #
    if (NOT cereal_POPULATED)
        find_package(cereal CONFIG QUIET)
        if (NOT cereal_FOUND)
            set(JUST_INSTALL_CEREAL TRUE)
            FetchContent_Declare(
                cereal
                GIT_REPOSITORY https://github.com/USCiLab/cereal.git
                GIT_TAG v1.3.2
            )
            FetchContent_MakeAvailable(cereal)
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
                    GIT_REPOSITORY https://github.com/bwpriest/ygm.git
                    GIT_TAG feature/cmake_3.30_tweaks
                )
                FetchContent_MakeAvailable(ygm)
            endif ()
        endif ()
    else ()
        message(STATUS ${PROJECT_NAME} " building without ygm")
    endif ()
else ()
    message(STATUS ${PROJECT_NAME} " building without cereal")
endif ()

add_library(krowkee INTERFACE)
add_library(krowkee::krowkee ALIAS krowkee)
target_include_directories(
    krowkee INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                      $<INSTALL_INTERFACE:include>
)
target_compile_features(krowkee INTERFACE cxx_std_20)
if (KROWKEE_USE_CEREAL)
    target_link_libraries(krowkee INTERFACE ${KROWKEE_CEREAL_TARGET})
    if (KROWKEE_USE_YGM)
        target_link_libraries(krowkee INTERFACE ygm::ygm)
    endif ()
endif ()
if (Boost_FOUND)
    target_include_directories(krowkee INTERFACE ${Boost_INCLUDE_DIRS})
endif ()

if (KROWKEE_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    set(KROWKEE_EXPORT_TARGETS krowkee)

    install(
        TARGETS ${KROWKEE_EXPORT_TARGETS}
        EXPORT ${PROJECT_NAME}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(
        EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake
    )

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/krowkee
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # create version file
    write_basic_package_version_file(
        "${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY ExactVersion
    )

    # create config file
    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake
    )

    install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake
    )

endif ()

if (JUST_INSTALL_KROWKEE)
    return()
endif ()

if (KROWKEE_MAIN_PROJECT)
    if (KROWKEE_BUILD_DOCS)
        add_subdirectory(docs)
    endif ()
    add_subdirectory(examples)
    add_subdirectory(test)
    add_subdirectory(performance)
endif ()
