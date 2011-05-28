cmake_minimum_required(VERSION 2.0)

# declare project
project(lpicp)

# set include directories
include_directories("${CMAKE_SOURCE_DIR}/inc" "${CMAKE_SOURCE_DIR}/pkg/inc" "~/dev/linux-2.6/denx/include")

# create library
add_library (lpicp src/lpicp.c src/lpicp_icsp.c src/lpicp_log.c src/lpicp_image.c pkg/src/ihex.c
             src/lpicp_device src/devices/18f/lpicp_dev_18f_2xx_4xx.c 
             src/devices/18f/lpicp_dev_18f_2xxx_4xxx.c)

# create lpicp executable
add_executable(lpicp-bin main.c)
set_target_properties(lpicp-bin PROPERTIES OUTPUT_NAME lpicp)

# link application to lib
target_link_libraries(lpicp-bin lpicp)
