file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS *.cpp)

add_executable(client ${CLIENT_SOURCES})

target_link_libraries(client PRIVATE common raylib)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/game/src/client)

set_target_properties(client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/client/$<CONFIG>
)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set_target_properties(client PROPERTIES SUFFIX ".html")
    target_link_options(client PRIVATE
        "-sUSE_GLFW=3"
        "-sWASM=1"
        "-sASYNCIFY"
        "-sALLOW_MEMORY_GROWTH=1"
        "--preload-file resources"
        "--shell-file=${CMAKE_SOURCE_DIR}/game/src/client/minshell.html"
    )
elseif(APPLE)
    target_link_libraries(client PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()