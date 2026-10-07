cmake_minimum_required(VERSION 3.22.1)
project(unlock_uwb)

set(MODULE_NAME "unlock_uwb")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CXX_FLAGS "${CXX_FLAGS} -fno-exceptions -fno-rtti -fvisibility=hidden -fvisibility-inlines-hidden")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLAGS}")

set(DOBBY_GENERATE_SHARED OFF CACHE INTERNAL "" FORCE)
set(DOBBY_DEBUG OFF CACHE INTERNAL "" FORCE)
set(LSPLANT_BUILD_SHARED OFF CACHE INTERNAL "" FORCE)

add_subdirectory(external/dobby)
add_subdirectory(external/lsplant/lsplant/src/main/jni lsplant_build)

target_compile_definitions(lsplant_static PRIVATE LOG_DISABLED LOG_TAG=\"unlock_uwb\")

add_library(xdl STATIC
    external/xdl/xdl/src/main/cpp/xdl.c
    external/xdl/xdl/src/main/cpp/xdl_iterate.c
    external/xdl/xdl/src/main/cpp/xdl_linker.c
    external/xdl/xdl/src/main/cpp/xdl_lzma.c
    external/xdl/xdl/src/main/cpp/xdl_util.c
)
target_include_directories(xdl PUBLIC external/xdl/xdl/src/main/cpp/include)

add_library(${MODULE_NAME} SHARED
    jni/main.cpp
    jni/lsplant_init.cpp
    jni/java_hooks.cpp
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/jni
    external/lsplant/lsplant/src/main/jni/include
    external/dobby/include
)

target_link_libraries(${MODULE_NAME} PRIVATE
    lsplant_static
    dobby
    xdl
    log
)
