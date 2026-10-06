# Example B: the hosted-layers demonstration as a bare-metal x86_64 kernel that Limine boots
# (examples/hosted-layers/README.md). libycxx is built with YCXX_PAL=none and the layers abort,
# memory, console and clock, which the kernel provides itself (providers.c); there is no OS and no
# C library. Configure with the toolchain file, then build kernel.elf:
#   cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -DYCXX_COMPILER=gcc
#   cmake --build build
# run.sh does that, makes a bootable ISO with Limine and boots it in QEMU.
cmake_minimum_required(VERSION 3.28)
project(hosted_layers_limine LANGUAGES C CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
  message(FATAL_ERROR "Configure Example B with its toolchain file: -DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/toolchain.cmake")
endif()

set(LIBYCXX_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." CACHE PATH "libycxx checkout")

# No platform layer from libycxx, no C library: the kernel provides these layers.
set(YCXX_PAL none)
set(YCXX_HOSTED_LAYERS abort memory console clock)
add_subdirectory(${LIBYCXX_SOURCE_DIR} libycxx EXCLUDE_FROM_ALL)

# The kernel's C code is freestanding too: the compiler's own headers (<stddef.h>, <stdint.h>)
# and no C library's.
execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=include OUTPUT_VARIABLE cc_include
                OUTPUT_STRIP_TRAILING_WHITESPACE)
set(kernel_c_options -ffreestanding -nostdinc -isystem${cc_include})

# The providers of the four layers: COM1, a heap in the Limine memory map, the clocks, abort.
add_library(kernel_providers STATIC providers.c hw.c ../common/heap.c)
target_include_directories(kernel_providers PRIVATE ${LIBYCXX_SOURCE_DIR}/include)
target_compile_options(kernel_providers PRIVATE ${kernel_c_options})
set_target_properties(kernel_providers PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED ON)
foreach(layer IN ITEMS abort memory console clock)
  ycxx_add_hosted_layer(${layer} PROVIDER kernel_providers)
endforeach()

# GCC 16's libgcc.a and libgcc_eh.a (the unwinder), with either compiler: the GCC compiling this,
# else $YCXX_GCC_INSTALL_DIR, else $YCXX_GXX or g++-16.
if(NOT DEFINED YCXX_LIBGCC_DIR)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(gxx "${CMAKE_CXX_COMPILER}")
  elseif(DEFINED ENV{YCXX_GCC_INSTALL_DIR})
    set(libgcc_dir "$ENV{YCXX_GCC_INSTALL_DIR}")
  elseif(DEFINED ENV{YCXX_GXX})
    set(gxx "$ENV{YCXX_GXX}")
  else()
    set(gxx g++-16)
  endif()
  if(NOT libgcc_dir)
    execute_process(COMMAND ${gxx} -print-libgcc-file-name OUTPUT_VARIABLE libgcc OUTPUT_STRIP_TRAILING_WHITESPACE)
    get_filename_component(libgcc_dir "${libgcc}" DIRECTORY)
  endif()
  set(YCXX_LIBGCC_DIR "${libgcc_dir}" CACHE PATH "Directory of GCC 16's libgcc.a and libgcc_eh.a")
endif()
if(NOT EXISTS "${YCXX_LIBGCC_DIR}/libgcc_eh.a")
  message(FATAL_ERROR "No libgcc_eh.a in YCXX_LIBGCC_DIR (${YCXX_LIBGCC_DIR}): set it to GCC 16's library directory")
endif()

add_executable(kernel.elf boot.c support.c main.cpp ../common/demo.cpp)
set_source_files_properties(boot.c support.c PROPERTIES COMPILE_OPTIONS "${kernel_c_options}")
# support.c defines memcpy and friends: GCC must not turn their loops into calls to themselves.
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  set_property(SOURCE support.c APPEND PROPERTY COMPILE_OPTIONS -fno-tree-loop-distribute-patterns)
endif()
set_target_properties(kernel.elf PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED ON LINK_DEPENDS
                      ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)
target_include_directories(kernel.elf PRIVATE ${LIBYCXX_SOURCE_DIR}/include)
target_link_libraries(kernel.elf PRIVATE ycxx::ycxx)
# The unwinder and libgcc go after libycxx's archives, which use them: as dependencies of the
# providers, which ycxx::ycxx brings last.
target_link_libraries(kernel_providers INTERFACE ${YCXX_LIBGCC_DIR}/libgcc_eh.a ${YCXX_LIBGCC_DIR}/libgcc.a)
# A static executable at the kernel's address (linker.ld), with the .eh_frame_hdr the unwinder
# finds through _dl_find_object (support.c); nothing from the host's C runtime.
target_link_options(kernel.elf PRIVATE -nostdlib -static -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
                    -Wl,--eh-frame-hdr -Wl,-z,max-page-size=0x1000 -Wl,--build-id=none)
