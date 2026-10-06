# Example A: a program of the host OS that uses libycxx without its POSIX platform layer. The
# program supplies the hosted layers it needs (providers.c: abort, memory, console, clock); libycxx
# builds only what those layers enable (examples/hosted-layers/README.md):
#   cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc-16 -DCMAKE_CXX_COMPILER=g++-16
#   cmake --build build && build/hosted_layers_host
# The absent_* targets use layers this build does not have and must fail to build:
#   cmake --build build --target absent_thread        # static_assert naming 'threads'
cmake_minimum_required(VERSION 3.28)
project(hosted_layers_host LANGUAGES C CXX)

set(LIBYCXX_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." CACHE PATH "libycxx checkout")

# No platform layer from libycxx: the layers below come from this program.
set(YCXX_PAL none)
set(YCXX_HOSTED_LAYERS abort memory console clock)
add_subdirectory(${LIBYCXX_SOURCE_DIR} libycxx EXCLUDE_FROM_ALL)

# The providers: plain C on the host's C library (the program's own; libycxx's build does not
# depend on the C library).
add_library(host_providers STATIC providers.c ../common/heap.c)
target_include_directories(host_providers PRIVATE ${LIBYCXX_SOURCE_DIR}/include)
set_target_properties(host_providers PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED ON)
foreach(layer IN ITEMS abort memory console clock)
  ycxx_add_hosted_layer(${layer} PROVIDER host_providers)
endforeach()

add_executable(hosted_layers_host main.cpp ../common/demo.cpp)
target_link_libraries(hosted_layers_host PRIVATE ycxx::ycxx)

# Programs that use layers this build does not provide: building them fails, saying which layer is
# missing (tests/cmake/run.sh checks the diagnostics).
foreach(absent IN ITEMS thread sleep random_device fstream)
  add_executable(absent_${absent} EXCLUDE_FROM_ALL absent/${absent}.cpp)
  target_link_libraries(absent_${absent} PRIVATE ycxx::ycxx)
endforeach()
