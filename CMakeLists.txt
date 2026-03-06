cmake_minimum_required(VERSION 3.25)
project(io-game-framework LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Global settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_CLIENT "Build client executable" ON)
option(BUILD_SERVER "Build server executable" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

# Force static linking for self-contained executables
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_link_options(-static-libstdc++)
elseif(MSVC)
    add_compile_options(/W4 /permissive-)
endif()

# ASan
if(ENABLE_ASAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    elseif(MSVC)
        add_compile_options(/fsanitize=address)
    endif()
endif()

# Build game components
add_subdirectory(game)