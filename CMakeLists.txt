# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

set(COMPONENTS main process_manager temperature_sensor wifi http_server spiffs)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(blink)

# spiffs_create_partition_image(www data FLASH_IN_PROJECT)
