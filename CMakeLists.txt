cmake_minimum_required(VERSION 3.28)
project(libycxx LANGUAGES C CXX)

# libycxx builds its own runtime with itself: no system C++ headers or library.
# The C++ standard flag is given explicitly: CMake does not know every compiler's C++26 flag.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_C_STANDARD 23)

set(YCXX_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# One replaceable allocation function per source file (see src/runtime/new).
file(GLOB YCXX_NEW_SOURCES CONFIGURE_DEPENDS src/runtime/new/*.cpp src/hosted/new/*.cpp)
# Defining one deallocation function per file is the point; GCC warns about each unpaired one.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set_source_files_properties(${YCXX_NEW_SOURCES} PROPERTIES COMPILE_OPTIONS -Wno-sized-deallocation)
endif()
add_library(ycxx STATIC
  ${YCXX_NEW_SOURCES}
  src/hosted/stdexcept.cpp
  src/pal/posix/pal.c
)
target_include_directories(ycxx SYSTEM PUBLIC ${YCXX_INCLUDE})
target_compile_options(ycxx PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-std=c++26 -nostdinc++ -Wall -Wextra -Werror>
  $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra -Werror>)

# The Itanium C++ ABI runtime (exception handling, RTTI, guards). Built with RTTI and
# exceptions; replaces the toolchain's libsupc++.
file(GLOB YCXX_ABI_SOURCES CONFIGURE_DEPENDS src/abi/*.cpp)
add_library(ycxx_abi STATIC ${YCXX_ABI_SOURCES})
set_target_properties(ycxx_abi PROPERTIES OUTPUT_NAME ycxx-abi)
target_include_directories(ycxx_abi SYSTEM PUBLIC ${YCXX_INCLUDE})
target_compile_options(ycxx_abi PRIVATE -std=c++26 -nostdinc++ -Wall -Wextra -Werror -fexceptions -frtti)
set_source_files_properties(src/abi/exception_classes.cpp PROPERTIES COMPILE_DEFINITIONS YCXX_EXCEPTION_KEY_FUNCTIONS)
