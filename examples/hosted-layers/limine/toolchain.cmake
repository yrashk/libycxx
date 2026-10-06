# CMake toolchain file for Example B's kernel: bare x86_64, no operating system, with the host's
# GCC 16.2 or Clang 23.1 (-DYCXX_COMPILER=gcc|clang; the compilers come from $YCXX_GCC/$YCXX_GXX
# or $YCXX_CLANG/$YCXX_CLANGXX, as with tools/toolchain/activate.sh, else gcc-16/g++-16 or
# clang-23/clang++-23). The host compilers target x86_64 ELF; for a kernel they get:
#   -mno-red-zone             interrupt and exception frames may land below %rsp
#   -fno-stack-protector, -fno-stack-check, -fcf-protection=none
#                             no canary from a C library, no probes, no CET markers
#   -march=x86-64             the baseline instruction set: SSE2, which boot.c enables
# and position-independent code (CMAKE_POSITION_INDEPENDENT_CODE; libycxx's archives are PIC
# anyway): the small code model, linked at 0xffffffff80000000 (linker.ld). Programs and libycxx
# itself are compiled freestanding: libycxx's ycxx::headers adds -ffreestanding -nostdinc for a
# build without the C library layer, and CMakeLists.txt does the same for the kernel's C files.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
# Nothing can be linked without the kernel's linker script: probes build static libraries.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(NOT DEFINED YCXX_COMPILER)
  set(YCXX_COMPILER gcc)
endif()
if(YCXX_COMPILER STREQUAL "gcc")
  set(_c "$ENV{YCXX_GCC}")
  set(_cxx "$ENV{YCXX_GXX}")
  if(NOT _c)
    set(_c gcc-16)
  endif()
  if(NOT _cxx)
    set(_cxx g++-16)
  endif()
elseif(YCXX_COMPILER STREQUAL "clang")
  set(_c "$ENV{YCXX_CLANG}")
  set(_cxx "$ENV{YCXX_CLANGXX}")
  if(NOT _c)
    set(_c clang-23)
  endif()
  if(NOT _cxx)
    set(_cxx clang++-23)
  endif()
else()
  message(FATAL_ERROR "toolchain.cmake: YCXX_COMPILER must be gcc or clang (got '${YCXX_COMPILER}')")
endif()
set(CMAKE_C_COMPILER "${_c}")
set(CMAKE_CXX_COMPILER "${_cxx}")

set(_kernel_flags "-mno-red-zone -fno-stack-protector -fno-stack-check -fcf-protection=none -march=x86-64")
set(CMAKE_C_FLAGS_INIT "${_kernel_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_kernel_flags}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
