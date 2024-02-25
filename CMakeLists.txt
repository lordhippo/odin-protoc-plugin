cmake_minimum_required(VERSION 3.25)

set(USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(protoc-gen-odin)

find_package(Protobuf CONFIG REQUIRED)

set(HEADER_FILES
    source/pch.h
    source/odin-code-generator.h)

set(SOURCE_FILES
    source/main.cpp
    source/odin-code-generator.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${PROJECT_NAME} 
    protobuf::libprotobuf
    protobuf::libprotoc)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES} ${HEADER_FILES})

target_precompile_headers(${PROJECT_NAME} PUBLIC "source/pch.h")

install(TARGETS ${PROJECT_NAME})

set(CPACK_PACKAGE_NAME "odin-protoc-plugin")
set(CPACK_PACKAGE_VERSION "0.1.0") # use git tag instead
set(CPACK_SYSTEM_NAME ${VCPKG_TARGET_TRIPLET})
include(CPack)
