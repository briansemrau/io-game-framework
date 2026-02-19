cmake_minimum_required(VERSION 3.20)
project(destruction_derby CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(thirdparty/raylib)
add_subdirectory(thirdparty/box2d)

# Collect source files from src directory manually to avoid subdirectory issues
file(GLOB_RECURSE SRC_SOURCE_FILES CONFIGURE_DEPENDS src/*.c src/*.cpp)
file(GLOB_RECURSE SRC_HEADER_FILES CONFIGURE_DEPENDS src/*.h src/*.hpp)

# Create executable with all source files (main.cpp is included in src files)
set(SOURCES ${SRC_SOURCE_FILES})
add_executable(${PROJECT_NAME} ${SOURCES})

# set_target_properties(${PROJECT_NAME} PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE raylib box2d)

# Add compiler flags for modern C++
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
endif()

# Web Configurations
if ("${PLATFORM}" STREQUAL "Web")
    # Tell Emscripten to build an example.html file.
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
    target_link_options(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-sUSE_GLFW=3 --preload-file resources -sWASM=1 -sASYNCIFY -sALLOW_MEMORY_GROWTH=1 --shell-file ${CMAKE_SOURCE_DIR}/minshell.html")
endif()

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
