# Example C: a 'files' hosted layer of the program's own (examples/hosted-layers/README.md). The
# host's C library is a layer here ('clib'), so iostreams work; the file streams run on ramdisk.c
# instead of the host's file system; memory and abort come from Example A's providers.
#   cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc-16 -DCMAKE_CXX_COMPILER=g++-16
#   cmake --build build && build/hosted_layers_files
cmake_minimum_required(VERSION 3.28)
project(hosted_layers_files LANGUAGES C CXX)

set(LIBYCXX_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." CACHE PATH "libycxx checkout")

set(YCXX_PAL none)
set(YCXX_HOSTED_LAYERS clib memory files)
add_subdirectory(${LIBYCXX_SOURCE_DIR} libycxx EXCLUDE_FROM_ALL)

add_library(files_providers STATIC ../host/providers.c ../common/heap.c)
target_include_directories(files_providers PRIVATE ${LIBYCXX_SOURCE_DIR}/include)
set_target_properties(files_providers PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED ON)
ycxx_add_hosted_layer(abort PROVIDER files_providers)
ycxx_add_hosted_layer(memory PROVIDER files_providers)
# The file layer, from sources: libycxx makes them a static library (ycxx_layer_files).
ycxx_add_hosted_layer(files SOURCES ramdisk.c)
set_target_properties(ycxx_layer_files PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED ON)
# ramdisk.c grows its files on the program's heap (heap.c, in files_providers).
target_link_libraries(ycxx_layer_files PRIVATE files_providers)

add_executable(hosted_layers_files main.cpp)
target_link_libraries(hosted_layers_files PRIVATE ycxx::ycxx)
