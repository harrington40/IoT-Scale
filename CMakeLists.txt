# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(scale_simple)
# ——— Doxygen integration —————————————
find_package(Doxygen QUIET)
if (DOXYGEN_FOUND)
    message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE}")  # sanity check

    set(DOXYFILE_IN   ${CMAKE_SOURCE_DIR}/Doxyfile)
    set(DOXYFILE_OUT  ${CMAKE_BINARY_DIR}/Doxyfile)

    # copy & replace @ONLY variables (none by default)
    configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

    # define a custom target you can invoke via `ninja docs`
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
else()
    message(STATUS "Doxygen not found; 'docs' target will be unavailable")
endif()
# ——————————————————————————————————————————
