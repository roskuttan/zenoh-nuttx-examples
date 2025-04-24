cmake_minimum_required(VERSION 3.28)
if(CONFIG_ROS2_SUB)
    set(CMAKE_SYSTEM_NAME "Generic")
    set(WITH_NUTTX ON CACHE BOOL "Build for NuttX RTOS" FORCE)
    set(ZENOH_LOG TRACE)
    #include_directories(${CMAKE_SOURCE_DIR}/apps/zenoh-pico-nuttx/include)
    add_subdirectory(../../zenoh-pico-nuttx SOURCE_DIR)
    add_compile_definitions(ZENOH_NUTTX)
    nuttx_add_application(
    NAME
    ${CONFIG_ROS2_SUB_PROGNAME}
    SRCS
    z_sub.c
    DEPENDS
    zenohpico_static)
endif()
