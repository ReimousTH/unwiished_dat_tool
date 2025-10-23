cmake_minimum_required(VERSION 3.8)
project(unwiished_dat_tool)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
    src/unwiished_dat_tool.cpp
    src/io/IWRData.cpp
    src/io/ISerializable.cpp
    src/io/FFile.cpp
    src/io_wii/DatFile.cpp
    src/system/Debug.cpp
)

set(HEADERS
    src/io/IWRData.h
    src/io/ISerializable.h
    src/io/FFile.h
    src/io_wii/DatFile.h
    src/system/Debug.h
    src/system_wii/Padding.h
)
add_executable(unwiished_dat_tool 
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(unwiished_dat_tool PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/rapidjson/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/xxHash
)

add_subdirectory(libs/xxHash/build/cmake)
add_subdirectory(scripts)
target_link_libraries(unwiished_dat_tool xxhash)

set(GENERATED_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/unwiished_dat_tool.h")

set(HEADER_CONTENT "// Auto-generated header file - do not edit manually!
#pragma once

")

foreach(header ${HEADERS})
    set(HEADER_CONTENT "${HEADER_CONTENT}#include \<${header}\>\n")
endforeach()

file(WRITE ${GENERATED_HEADER} "${HEADER_CONTENT}")

message(STATUS "Generated header: ${GENERATED_HEADER}")

