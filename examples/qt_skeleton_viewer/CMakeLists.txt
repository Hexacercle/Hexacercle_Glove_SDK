cmake_minimum_required(VERSION 3.16)
project(HexQtSkeletonViewer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 QUIET COMPONENTS Widgets)
if(Qt6_FOUND)
    set(QT_WIDGETS Qt6::Widgets)
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets)
    set(QT_WIDGETS Qt5::Widgets)
endif()

get_filename_component(PACKAGE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
if(EXISTS "${PACKAGE_ROOT}/接口文档/library/include/hex_glove/hex_mocap_api.h")
    set(SDK_LIBRARY_ROOT "${PACKAGE_ROOT}/接口文档/library")
elseif(EXISTS "${PACKAGE_ROOT}/外部接口文档/library/include/hex_glove/hex_mocap_api.h")
    set(SDK_LIBRARY_ROOT "${PACKAGE_ROOT}/外部接口文档/library")
else()
    message(FATAL_ERROR "找不到 SDK 外部头文件：请检查发布包中的 接口文档/library 或 外部接口文档/library")
endif()
add_library(HexaCercleGlove SHARED IMPORTED)
if(WIN32)
    if(NOT MSVC)
        message(FATAL_ERROR "This Windows package requires an MSVC x64 Qt kit (Visual Studio 2022).")
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Use the x64 platform: -A x64")
    endif()
    set(SDK_BIN "${SDK_LIBRARY_ROOT}/windows-x64")
    set(SDK_INCLUDE "${SDK_LIBRARY_ROOT}/include")
    set(UNITY_MAPPING_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows-x64/HexQtUnityMapping.lib")
    set_target_properties(HexaCercleGlove PROPERTIES
        IMPORTED_LOCATION "${SDK_BIN}/HexaCercleGlove.dll"
        IMPORTED_IMPLIB "${SDK_BIN}/HexaCercleGlove.lib")
else()
    set(SDK_BIN "${SDK_LIBRARY_ROOT}/linux-x64")
    set(SDK_INCLUDE "${SDK_BIN}/include")
    set(UNITY_MAPPING_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux-x64/libHexQtUnityMapping.a")
    set_target_properties(HexaCercleGlove PROPERTIES
        IMPORTED_LOCATION "${SDK_BIN}/libHexaCercleGlove.so")
endif()
set_target_properties(HexaCercleGlove PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SDK_INCLUDE}")

if(NOT EXISTS "${UNITY_MAPPING_LIBRARY}")
    message(FATAL_ERROR "找不到 Qt 示例映射库：${UNITY_MAPPING_LIBRARY}")
endif()
add_library(HexQtUnityMapping STATIC IMPORTED)
set_target_properties(HexQtUnityMapping PROPERTIES IMPORTED_LOCATION "${UNITY_MAPPING_LIBRARY}")

add_executable(qt_skeleton_viewer main.cpp)
set_target_properties(qt_skeleton_viewer PROPERTIES AUTOMOC ON)
target_link_libraries(qt_skeleton_viewer PRIVATE HexQtUnityMapping HexaCercleGlove ${QT_WIDGETS})
if(MSVC)
    target_compile_options(qt_skeleton_viewer PRIVATE /utf-8)
endif()
if(WIN32)
    file(GLOB SDK_MSVC_RUNTIME CONFIGURE_DEPENDS
        "${SDK_BIN}/msvcp140*.dll" "${SDK_BIN}/vcruntime140*.dll"
        "${SDK_BIN}/concrt140.dll")
    add_custom_command(TARGET qt_skeleton_viewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDK_BIN}/HexaCercleGlove.dll" ${SDK_MSVC_RUNTIME}
            "$<TARGET_FILE_DIR:qt_skeleton_viewer>"
        VERBATIM)
else()
    set_target_properties(qt_skeleton_viewer PROPERTIES BUILD_RPATH "${SDK_BIN}")
endif()
