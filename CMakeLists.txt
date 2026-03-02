cmake_minimum_required (VERSION 3.11)

project(tiny-vulkan-hpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

find_package(Vulkan REQUIRED)

add_library(tinyvkpp INTERFACE)
target_include_directories(tinyvkpp INTERFACE "include")
target_link_libraries(tinyvkpp INTERFACE Vulkan::Vulkan)

add_subdirectory(examples)