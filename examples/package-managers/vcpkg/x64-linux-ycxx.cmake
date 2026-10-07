# vcpkg overlay triplet: packages built against libycxx (docs/BUILDING_PROJECTS.md, "Package
# managers"). YCXX_PREFIX names a libycxx installation (cmake --install); its toolchain file is
# chainloaded, so every port built with CMake (and vcpkg's Meson and autotools ports, which take
# their compilers from it) uses libycxx's compilers. Use with
#   YCXX_PREFIX=/opt/libycxx cmake -S <project> -B <build> \
#     -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake \
#     -DVCPKG_OVERLAY_TRIPLETS=<libycxx>/examples/package-managers/vcpkg -DVCPKG_TARGET_TRIPLET=x64-linux-ycxx \
#     -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=/opt/libycxx/lib/cmake/libycxx/toolchain.cmake
# (the last line for the project itself; the triplet does the same for the ports).
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
# Static: each shared library linking libycxx has its own runtime (DECISIONS §2).
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_ENV_PASSTHROUGH YCXX_PREFIX)
if(NOT DEFINED ENV{YCXX_PREFIX})
  message(FATAL_ERROR "x64-linux-ycxx: set YCXX_PREFIX to a libycxx installation prefix")
endif()
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "$ENV{YCXX_PREFIX}/lib/cmake/libycxx/toolchain.cmake")
