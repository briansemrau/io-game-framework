
function(suppress_third_party_warnings)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs TARGETS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    foreach(target ${ARG_TARGETS})
        if(TARGET ${target})
            # Check if target is an INTERFACE library
            get_target_property(target_type ${target} TYPE)
            if("${target_type}" STREQUAL "INTERFACE_LIBRARY")
                set(scope INTERFACE)
            else()
                set(scope PRIVATE)
            endif()
            
            if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
                target_compile_options(${target} ${scope} -w)
            else()
                target_compile_options(${target} ${scope} /W0)
            endif()
        endif()
    endforeach()
endfunction()

# Dependencies - platform-agnostic
add_subdirectory(thirdparty/plog)
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(thirdparty/json)
add_subdirectory(thirdparty/box2d)
add_subdirectory(thirdparty/entt)
suppress_third_party_warnings(TARGETS box2d EnTT)

# Networking - platform-specific
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    add_subdirectory(thirdparty/datachannel-wasm)
    suppress_third_party_warnings(TARGETS datachannel-wasm)
else()
    set(NO_MEDIA ON CACHE BOOL "" FORCE)
    set(NO_EXAMPLES ON CACHE BOOL "" FORCE)
    set(NO_TESTS ON CACHE BOOL "" FORCE)
    set(USE_SYSTEM_PLOG ON CACHE BOOL "" FORCE)
    set(USE_SYSTEM_JSON ON CACHE BOOL "" FORCE)
    set(plog_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/game/thirdparty/plog/include)
    add_subdirectory(thirdparty/libdatachannel)
    suppress_third_party_warnings(TARGETS datachannel datachannel-static usrsctplib juice juice-static)
endif()

# Transport abstraction layer
add_library(transport INTERFACE)
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_link_libraries(transport INTERFACE datachannel-wasm)
else()
    target_link_libraries(transport INTERFACE datachannel)
endif()

# Common library
file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS src/common/*.cpp)

add_library(common STATIC ${COMMON_SOURCES})

target_link_libraries(common PUBLIC plog::plog nlohmann_json::nlohmann_json transport box2d EnTT::EnTT)
target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR}/game/thirdparty/zpp_bits)
target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR}/game/src/common)

# Client
if(BUILD_CLIENT)
    add_subdirectory(thirdparty/raylib)
    suppress_third_party_warnings(TARGETS raylib glfw)

    add_subdirectory(src/client)
endif()

# Server
if(BUILD_SERVER)
    add_subdirectory(src/server)
endif()