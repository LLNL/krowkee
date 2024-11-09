# find_package(Doxygen REQUIRED) find_package(Sphinx REQUIRED)

# # Find all the public headers get_target_property( KROWKEE_PUBLIC_HEADER_DIR
# krowkee INTERFACE_INCLUDE_DIRECTORIES ) file(GLOB_RECURSE
# KROWKEE_PUBLIC_HEADERS ${KROWKEE_PUBLIC_HEADER_DIR}/*.hpp)

# set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/include) set(DOXYGEN_OUTPUT_DIR
# ${CMAKE_CURRENT_BINARY_DIR}/doxygen) set(DOXYGEN_INDEX_FILE
# ${DOXYGEN_OUTPUT_DIR}/xml/index.xml) set(DOXYFILE_IN
# ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in) set(DOXYFILE_OUT
# ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

# # Replace variables inside @@ with the current values
# configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

# # Doxygen won't create this for us file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

# # Only regenerate Doxygen when the Doxyfile or public headers change
# add_custom_command( OUTPUT ${DOXYGEN_INDEX_FILE} DEPENDS
# ${KROWKEE_PUBLIC_HEADERS} COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
# MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN} COMMENT "Generating
# documentation with Doxygen" VERBATIM )

# # Nice named target so we can run the job easily add_custom_target(Doxygen ALL
# DEPENDS ${DOXYGEN_INDEX_FILE})

# set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}) set(SPHINX_BUILD
# ${CMAKE_CURRENT_BINARY_DIR}/sphinx) set(SPHINX_INDEX_FILE
# ${SPHINX_BUILD}/index.html)

# # Only regenerate Sphinx when: - Doxygen has rerun - Our doc files have been #
# updated - The Sphinx config has been updated add_custom_command( OUTPUT
# ${SPHINX_INDEX_FILE} COMMAND ${SPHINX_EXECUTABLE} -b html
# -Dbreathe_projects.krowkee=${DOXYGEN_OUTPUT_DIR}/xml ${SPHINX_SOURCE}
# ${SPHINX_BUILD} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} DEPENDS # Other
# docs files you want to track should go here (or in some # variable)
# ${CMAKE_CURRENT_SOURCE_DIR}/index.rst ${DOXYGEN_INDEX_FILE} MAIN_DEPENDENCY
# ${SPHINX_SOURCE}/conf.py COMMENT "Generating documentation with Sphinx" )

# # Nice named target so we can run the job easily add_custom_target(Sphinx ALL
# DEPENDS ${SPHINX_INDEX_FILE})

# # Add an install target to install the docs include(GNUInstallDirs)
# install(DIRECTORY ${SPHINX_BUILD} DESTINATION ${CMAKE_INSTALL_DOCDIR})

find_package(Doxygen REQUIRED)
find_package(Sphinx REQUIRED)

# Set all the public headers
set(KROWKEE_PUBLIC_HEADERS
    ${PROJECT_SOURCE_DIR}/include/krowkee/container/compacting_map.hpp
    ${PROJECT_SOURCE_DIR}/include/krowkee/hash/hash.hpp
    ${PROJECT_SOURCE_DIR}/include/krowkee/hash/util.hpp
    ${PROJECT_SOURCE_DIR}/include/krowkee/sketch/Dense.hpp
    ${PROJECT_SOURCE_DIR}/include/krowkee/sketch/Promotable.hpp
    ${PROJECT_SOURCE_DIR}/include/krowkee/sketch/Sketch.hpp
    ${PROJECT_SOURCE_DIR}/include/krowkee/sketch/Sparse.hpp
    ${PROJECT_SOURCE_DIR}/include/krowkee/sketch.hpp
    ${PROJECT_SOURCE_DIR}/include/krowkee/transform/Element.hpp
    ${PROJECT_SOURCE_DIR}/include/krowkee/transform/SparseJLT.hpp
    ${PROJECT_SOURCE_DIR}/include/krowkee/transform/FWHT.hpp
)

set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/include/)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/xml/index.xml)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

# Replace variables inside @@ with the current values
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

# Doxygen won't create this for us
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

# Only regenerate Doxygen when the Doxyfile or public headers change
add_custom_command(
    OUTPUT ${DOXYGEN_INDEX_FILE}
    DEPENDS ${KROWKEE_PUBLIC_HEADERS}
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
    MAIN_DEPENDENCY ${DOXYFILE_OUT}
    ${DOXYFILE_IN}
    COMMENT "Generating docs"
    VERBATIM
)

# Nice named target so we can run the job easily
add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)
set(SPHINX_SOURCE_FILES ${SPHINX_SOURCE}/index.rst ${SPHINX_SOURCE}/krowkee/*
                        ${SPHINX_SOURCE}/krowkee/*/*
)

# Only regenerate Sphinx when: - Doxygen has rerun - Our doc files have been
# updated - The Sphinx config has been updated
add_custom_command(
    OUTPUT ${SPHINX_INDEX_FILE}
    COMMAND
        ${SPHINX_EXECUTABLE} -b html
        # Tell Breathe where to find the Doxygen output
        -Dbreathe_projects.krowkee=${DOXYGEN_OUTPUT_DIR}/xml ${SPHINX_SOURCE}
        ${SPHINX_BUILD}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS # Other docs files you want to track should go here (or in some
            # variable)
            ${CMAKE_CURRENT_SOURCE_DIR}/index.rst
            ${SPHINX_SOURCE_FILES}
            ${DOXYGEN_INDEX_FILE}
    MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
    COMMENT "Generating documentation with Sphinx"
)

# Nice named target so we can run the job easily
add_custom_target(Sphinx ALL DEPENDS ${SPHINX_INDEX_FILE})

# Add an install target to install the docs
include(GNUInstallDirs)
install(DIRECTORY ${SPHINX_BUILD} DESTINATION ${CMAKE_INSTALL_DOCDIR})
