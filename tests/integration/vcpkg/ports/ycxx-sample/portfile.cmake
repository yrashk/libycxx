# A vcpkg overlay port built from local sources (tests/integration/cmake-project/sub): checks
# that the x64-linux-ycxx triplet (examples/package-managers/vcpkg) builds ports with libycxx's
# compilers without downloading anything.
set(SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../cmake-project/sub")
vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
file(INSTALL "${SOURCE_PATH}/sub.hpp" DESTINATION "${CURRENT_PACKAGES_DIR}/include")
file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright" "Test port of libycxx (tests/integration).\n")
