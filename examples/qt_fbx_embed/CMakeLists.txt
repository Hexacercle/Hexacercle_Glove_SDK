cmake_minimum_required(VERSION 3.16)
project(HexQtFbxEmbed LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This example requires 64-bit Windows and an MSVC x64 Qt kit.")
endif()
find_package(Qt6 6.5.3 EXACT REQUIRED COMPONENTS Widgets)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(sdk_library "${CMAKE_CURRENT_SOURCE_DIR}/../../接口文档/library")
set(sdk_bin "${sdk_library}/windows-x64")

add_executable(qt_fbx_embed main.cpp)
target_compile_options(qt_fbx_embed PRIVATE /utf-8)
target_include_directories(qt_fbx_embed PRIVATE "${sdk_library}/include")
target_link_libraries(qt_fbx_embed PRIVATE Qt6::Widgets
    "${sdk_bin}/HexQtFbxView.lib" "${sdk_bin}/HexaCercleGlove.lib")

file(GLOB sdk_runtime CONFIGURE_DEPENDS
    "${sdk_bin}/msvcp140*.dll" "${sdk_bin}/vcruntime140*.dll" "${sdk_bin}/concrt140.dll")
add_custom_command(TARGET qt_fbx_embed POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${sdk_bin}/HexQtFbxView.dll" "${sdk_bin}/HexaCercleGlove.dll"
        ${sdk_runtime} "$<TARGET_FILE_DIR:qt_fbx_embed>"
    VERBATIM)
