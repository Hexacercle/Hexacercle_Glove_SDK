cmake_minimum_required(VERSION 3.16)
project(HexSdkExamples LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(PACKAGE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(SDK_LIBRARY_ROOT "${PACKAGE_ROOT}/接口文档/library")
add_library(HexaCercleGlove SHARED IMPORTED)
if(WIN32)
    if(NOT MSVC)
        message(FATAL_ERROR "This Windows package requires MSVC x64 (Visual Studio 2022).")
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Use the x64 platform: -A x64")
    endif()
    set(SDK_BIN "${SDK_LIBRARY_ROOT}/windows-x64")
    set(SDK_INCLUDE "${SDK_LIBRARY_ROOT}/include")
    set_target_properties(HexaCercleGlove PROPERTIES
        IMPORTED_LOCATION "${SDK_BIN}/HexaCercleGlove.dll"
        IMPORTED_IMPLIB "${SDK_BIN}/HexaCercleGlove.lib")
    file(GLOB SDK_MSVC_RUNTIME CONFIGURE_DEPENDS
        "${SDK_BIN}/msvcp140*.dll" "${SDK_BIN}/vcruntime140*.dll"
        "${SDK_BIN}/concrt140.dll")
else()
    set(SDK_BIN "${SDK_LIBRARY_ROOT}/linux-x64")
    set(SDK_INCLUDE "${SDK_BIN}/include")
    set_target_properties(HexaCercleGlove PROPERTIES
        IMPORTED_LOCATION "${SDK_BIN}/libHexaCercleGlove.so")
endif()
set_target_properties(HexaCercleGlove PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SDK_INCLUDE}")

foreach(demo sdk_quickstart external_snapshot partner_workflow)
    add_executable(${demo} "${demo}/main.cpp")
    target_link_libraries(${demo} PRIVATE HexaCercleGlove)
    if(MSVC)
        target_compile_options(${demo} PRIVATE /utf-8)
    endif()
    if(WIN32)
        add_custom_command(TARGET ${demo} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SDK_BIN}/HexaCercleGlove.dll" ${SDK_MSVC_RUNTIME}
                "$<TARGET_FILE_DIR:${demo}>"
            VERBATIM)
    else()
        set_target_properties(${demo} PROPERTIES BUILD_RPATH "${SDK_BIN}")
    endif()
endforeach()
