file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS *.cpp)

add_executable(server ${SERVER_SOURCES})

target_link_libraries(server PRIVATE common)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/game/src/server)

set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/server/$<CONFIG>
)