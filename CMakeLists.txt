cmake_minimum_required(VERSION 3.25)
project(destruction_derby LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Global settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies - platform-agnostic
add_subdirectory(thirdparty/box2d)
add_subdirectory(thirdparty/entt)
add_subdirectory(thirdparty/raylib)

# Networking - platform-specific
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    add_subdirectory(thirdparty/datachannel-wasm)
else()
    set(NO_MEDIA ON CACHE BOOL "" FORCE)
    set(NO_EXAMPLES ON CACHE BOOL "" FORCE)
    set(NO_TESTS ON CACHE BOOL "" FORCE)
    add_subdirectory(thirdparty/libdatachannel)
endif()

# Transport abstraction layer
add_library(transport INTERFACE)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_link_libraries(transport INTERFACE datachannel-wasm)
else()
    target_link_libraries(transport INTERFACE datachannel)
endif()

# Compiler warnings
set(COMPILE_OPTIONS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    list(APPEND COMPILE_OPTIONS -Wall -Wextra -Wpedantic)
elseif(MSVC)
    list(APPEND COMPILE_OPTIONS /W4 /permissive-)
endif()

# ASan
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

if(ENABLE_ASAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    elseif(MSVC)
        add_compile_options(/fsanitize=address)
    endif()
endif()

# Shared sources
file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS src/common/*.cpp)

add_library(common STATIC ${COMMON_SOURCES})

target_link_libraries(common PUBLIC box2d transport)
target_include_directories(common PUBLIC thirdparty/zpp_bits)
target_include_directories(common PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common
)

target_compile_options(common PRIVATE ${COMPILE_OPTIONS})

# Client
file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS src/client/*.cpp)

add_executable(client ${CLIENT_SOURCES})

target_link_libraries(client PRIVATE common raylib)

target_include_directories(client PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client
)

target_compile_options(client PRIVATE ${COMPILE_OPTIONS})

set_target_properties(client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/client/$<CONFIG>
)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set_target_properties(client PROPERTIES SUFFIX ".html")
    target_link_options(${target} PRIVATE
        "-sUSE_GLFW=3"
        "-sWASM=1"
        "-sASYNCIFY"
        "-sALLOW_MEMORY_GROWTH=1"
        "--preload-file resources"
        "--shell-file=${CMAKE_SOURCE_DIR}/src/client/minshell.html"
    )
elseif(APPLE)
    target_link_libraries(client PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()

# Server
file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS src/server/*.cpp)

add_executable(server ${SERVER_SOURCES})

target_link_libraries(server PRIVATE common)

target_include_directories(server PRIVATE
    ${CMAKE_SOURCE_DIR}/src/server
)

target_compile_options(server PRIVATE ${COMPILE_OPTIONS})

set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/server/$<CONFIG>
)