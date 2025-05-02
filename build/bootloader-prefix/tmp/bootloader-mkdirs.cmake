# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/harrington/esp/v5.4.1/esp-idf/components/bootloader/subproject")
  file(MAKE_DIRECTORY "/home/harrington/esp/v5.4.1/esp-idf/components/bootloader/subproject")
endif()
file(MAKE_DIRECTORY
  "/home/harrington/scale_iot_design_04-30-2025/build/bootloader"
  "/home/harrington/scale_iot_design_04-30-2025/build/bootloader-prefix"
  "/home/harrington/scale_iot_design_04-30-2025/build/bootloader-prefix/tmp"
  "/home/harrington/scale_iot_design_04-30-2025/build/bootloader-prefix/src/bootloader-stamp"
  "/home/harrington/scale_iot_design_04-30-2025/build/bootloader-prefix/src"
  "/home/harrington/scale_iot_design_04-30-2025/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/harrington/scale_iot_design_04-30-2025/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/harrington/scale_iot_design_04-30-2025/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
