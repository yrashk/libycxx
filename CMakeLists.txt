cmake_minimum_required(VERSION 3.28)
project(libycxx LANGUAGES C CXX)

# libycxx builds its own runtime with itself: no system C++ headers or library.
# The C++ standard flag is given explicitly: CMake does not know every compiler's C++26 flag.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_C_STANDARD 23)

set(YCXX_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(ycxx STATIC
  src/hosted/new.cpp
  src/hosted/stdexcept.cpp
  src/pal/posix/pal.c
)
target_include_directories(ycxx SYSTEM PUBLIC ${YCXX_INCLUDE})
target_compile_options(ycxx PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-std=c++26 -nostdinc++ -Wall -Wextra -Werror>
  $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra -Werror>)
