cmake_minimum_required(VERSION 3.25)
project(destruction_derby CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(thirdparty/raylib)
add_subdirectory(thirdparty/box2d)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/src/common
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(COMPILE_OPTIONS -Wall -Wextra -pedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(COMPILE_OPTIONS /W4 /permissive-)
endif()

function(add_client_executable TARGET)
    add_executable(${TARGET} ${ARGN})
    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/client
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/client
    )
    target_link_libraries(${TARGET} PRIVATE raylib box2d)
    target_compile_options(${TARGET} PRIVATE ${COMPILE_OPTIONS})
    if (APPLE)
        target_link_libraries(${TARGET} PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
    endif()
    if ("${PLATFORM}" STREQUAL "Web")
        set_target_properties(${TARGET} PROPERTIES SUFFIX ".html")
        target_link_options(${TARGET} PROPERTIES LINK_FLAGS "-sUSE_GLFW=3 --preload-file resources -sWASM=1 -sASYNCIFY -sALLOW_MEMORY_GROWTH=1 --shell-file ${CMAKE_SOURCE_DIR}/src/client/minshell.html")
    endif()
endfunction()

function(add_server_executable TARGET)
    add_executable(${TARGET} ${ARGN})
    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/server
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/server
    )
    target_link_libraries(${TARGET} PRIVATE box2d)
    target_compile_options(${TARGET} PRIVATE ${COMPILE_OPTIONS})
endfunction()

file(GLOB_RECURSE CLIENT_SOURCES "${CMAKE_SOURCE_DIR}/src/client/*.cpp")
file(GLOB_RECURSE COMMON_SOURCES "${CMAKE_SOURCE_DIR}/src/common/*.cpp")

add_client_executable(client ${CLIENT_SOURCES} ${COMMON_SOURCES})

file(GLOB_RECURSE SERVER_SOURCES "${CMAKE_SOURCE_DIR}/src/server/*.cpp")
add_server_executable(server ${SERVER_SOURCES} ${COMMON_SOURCES})
