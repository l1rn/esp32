# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

add_compile_options("-include" "${CMAKE_CURRENT_SOURCE_DIR}/main/common_types.h")
spiffs_create_partition_image(www ../wifi_configs FLASH_IN_PROJECT)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(blink)
