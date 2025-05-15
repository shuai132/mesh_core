cmake_minimum_required(VERSION 3.15)

project(mesh_core)

# option
option(MESH_CORE_ENABLE_ROUTE_DEBUG "" OFF)
option(MESH_CORE_ENABLE_TIME_SYNC "" OFF)
option(MESH_CORE_ENABLE_BROADCAST_INTERCEPTOR "" OFF)
option(MESH_CORE_ENABLE_DISPATCH_INTERCEPTOR "" OFF)
option(MESH_CORE_DISABLE_ROUTE "" OFF)

# test
option(MESH_CORE_BUILD_TEST "" OFF)
option(MESH_CORE_BUILD_TEST_ALL_FUTURE "" ON)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MESH_CORE_BUILD_TEST ON)
endif ()

set(CMAKE_CXX_STANDARD 14)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

if (MESH_CORE_ENABLE_ROUTE_DEBUG)
    target_compile_definitions(${PROJECT_NAME} INTERFACE -DMESH_CORE_ENABLE_ROUTE_DEBUG)
endif ()
if (MESH_CORE_ENABLE_TIME_SYNC)
    target_compile_definitions(${PROJECT_NAME} INTERFACE -DMESH_CORE_ENABLE_TIME_SYNC)
endif ()
if (MESH_CORE_ENABLE_BROADCAST_INTERCEPTOR)
    target_compile_definitions(${PROJECT_NAME} INTERFACE -DMESH_CORE_ENABLE_BROADCAST_INTERCEPTOR)
endif ()
if (MESH_CORE_ENABLE_DISPATCH_INTERCEPTOR)
    target_compile_definitions(${PROJECT_NAME} INTERFACE -DMESH_CORE_ENABLE_DISPATCH_INTERCEPTOR)
endif ()
if (MESH_CORE_DISABLE_ROUTE)
    target_compile_definitions(${PROJECT_NAME} INTERFACE -DMESH_CORE_DISABLE_ROUTE)
endif ()

if (MESH_CORE_BUILD_TEST)
    add_definitions(-DMESH_CORE_LOG_SHOW_DEBUG)
    # add_definitions(-DMESH_CORE_LOG_SHOW_VERBOSE)

    # config for test
    add_definitions(-DMESH_CORE_DEBUG_TS_SAVE_SEND_FROM)
    add_definitions(-DMESH_CORE_ROUTE_EXPIRED_MS=30000)
    add_definitions(-DMESH_CORE_ROUTE_SYNC_INTERVAL_MS=10000)
    if (MESH_CORE_BUILD_TEST_ALL_FUTURE)
        message(STATUS "mesh_core: enable all future")
        add_definitions(-DMESH_CORE_ENABLE_ROUTE_DEBUG)
        add_definitions(-DMESH_CORE_ENABLE_TIME_SYNC)
        add_definitions(-DMESH_CORE_ENABLE_BROADCAST_INTERCEPTOR)
        add_definitions(-DMESH_CORE_ENABLE_DISPATCH_INTERCEPTOR)
    else ()
        message(STATUS "mesh_core: disable all future")
    endif ()

    message(STATUS "ASIO_PATH: $ENV{ASIO_PATH}")
    add_definitions(-DASIO_NO_DEPRECATED)
    include_directories($ENV{ASIO_PATH})

    if (MSVC)
        add_compile_options(/Zc:preprocessor)
        add_compile_options(/utf-8)
        add_compile_options(-DNOMINMAX)
    else ()
        add_compile_options(-Wall -Wunused-parameter)
    endif ()

    if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
        add_compile_options(-D_WIN32_WINNT=0x0601)
    endif ()

    if (MINGW)
        link_libraries(ws2_32 wsock32)
    endif ()

    # for android standalone e.g. termux
    add_definitions(-DANDROID_STANDALONE)

    set(TARGET_NAME ${PROJECT_NAME}_usage)
    add_executable(${TARGET_NAME} test/usage.cpp)
    target_link_libraries(${TARGET_NAME} ${PROJECT_NAME})

    set(TARGET_NAME ${PROJECT_NAME}_unittest)
    add_executable(${TARGET_NAME} test/unittest.cpp)
    target_link_libraries(${TARGET_NAME} ${PROJECT_NAME})

    if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(TARGET_NAME ${PROJECT_NAME}_udp_mesh)
        add_executable(${TARGET_NAME} test/udp_mesh.cpp)
        target_link_libraries(${TARGET_NAME} ${PROJECT_NAME})
    endif ()
endif ()
