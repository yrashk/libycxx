#!/bin/sh
# Builds projects that know nothing about libycxx against an installed libycxx, every way
# docs/BUILDING_PROJECTS.md documents, and proves each build is libycxx's (tools/ycxx-check-binary):
#   1. the installed files: bin/ycxx-c++, bin/ycxx-cc, bin/ycxx-check-binary,
#      lib/cmake/libycxx/toolchain.cmake, lib/pkgconfig/libycxx.pc, share/libycxx/meson-native.ini;
#   2. ycxx-check-binary rejects a program built with the toolchain's own C++ library;
#   3. cmake-project (a pinned C++17, try_compile checks, a shared library with C sources, a
#      FetchContent and an ExternalProject dependency, install(EXPORT)) with the toolchain file
#      alone, with Ninja and with Makefiles: builds, its test passes, installs, and cmake-consumer
#      builds against the installed package;
#   4. the same with CC/CXX in the environment instead of a toolchain file;
#   5. make-project: CXX=ycxx-c++, and the plain compiler with pkg-config's libycxx.pc;
#   6. the installation moved elsewhere still works (wrappers and libycxx.pc are relocatable);
#   7. activate.sh --use <prefix> (bash): CXX/CC/PKG_CONFIG_PATH for make;
#   8. meson-project with the Meson native file (meson, or uvx's; skipped without either);
#   9. autotools-project with ./configure CXX=ycxx-c++ (skipped without autoreconf).
#
#   tests/integration/run.sh gcc|clang PREFIX [WORKDIR]
# PREFIX: libycxx installed by `cmake --install` with that compiler (tests/cmake/run.sh passes the
# one it installs). Exit status 1 when a check failed.
set -eu
repo=$(cd "$(dirname "$0")/../.." && pwd)
. "$repo/tools/lib/ui.sh"
. "$repo/tools/lib/env.sh"
ycxx_env_load
c=${1:?usage: tests/integration/run.sh gcc|clang PREFIX [WORKDIR]}
prefix=$(cd "${2:?usage: tests/integration/run.sh gcc|clang PREFIX [WORKDIR]}" && pwd)
work=${3:-$repo/build/integration-test/$c}
src=$repo/tests/integration
case $c in
  gcc) cc=${YCXX_GCC:-gcc-16} cxx=${YCXX_GXX:-g++-16} ;;
  clang) cc=${YCXX_CLANG:-clang-23} cxx=${YCXX_CLANGXX:-clang++-23} ;;
  *) echo "tests/integration/run.sh: unknown compiler $c" >&2; exit 2 ;;
esac
rm -rf "$work"
mkdir -p "$work"
log=$work/log.txt
ui_info "log" "$log"
fail=0
ok() { ui_ok "[$c] $1"; }
bad() { ui_fail "[$c] $1"; fail=1; }
# x CMD...: show the command, run it with its output appended to $log.
x() { ui_cmd "$@"; "$@" >>"$log" 2>&1; }
check=$prefix/bin/ycxx-check-binary

# expect_output EXE TEXT: EXE runs, exits 0 and prints TEXT (as its last line).
expect_output() {
  e_st=0
  e_out=$("$1" 2>&1) || e_st=$?
  printf '$ %s\n%s\n[exit %s]\n' "$1" "$e_out" "$e_st" >>"$log"
  [ "$e_st" = 0 ] && [ "$(printf '%s\n' "$e_out" | tail -n 1)" = "$2" ]
}

# libycxx_only WHAT PATH...: every binary under PATH is libycxx's, else a failure.
libycxx_only() {
  l_what=$1
  shift
  if "$check" "$@" >>"$log" 2>&1; then ok "$l_what: built against libycxx alone (ycxx-check-binary)"
  else bad "$l_what: ycxx-check-binary found binaries not built against libycxx (see $log)"; fi
}

# 1. the installed files
missing=
for f in bin/ycxx-c++ bin/ycxx-cc bin/ycxx-check-binary lib/cmake/libycxx/toolchain.cmake \
         lib/pkgconfig/libycxx.pc share/libycxx/meson-native.ini share/libycxx/python/ycxx_linkage.py; do
  [ -e "$prefix/$f" ] || missing="$missing $f"
done
if [ -z "$missing" ]; then ok "installed: compilers, toolchain file, libycxx.pc, Meson native file, ycxx-check-binary"
else bad "installed files missing:$missing"; fi
toolchain=$prefix/lib/cmake/libycxx/toolchain.cmake

# 2. the checker rejects the toolchain's own library (make-project with the plain compiler)
b=$work/plain
mkdir -p "$b"
if x make -C "$b" -f "$src/make-project/Makefile" VPATH="$src/make-project" CXX="$cxx" CC="$cc" "CXXFLAGS=-O1 -std=c++26"; then
  if "$check" "$b/app" >>"$log" 2>&1; then bad "ycxx-check-binary accepted a program built with the toolchain's C++ library"
  else ok "ycxx-check-binary rejects a program built with the toolchain's C++ library"; fi
else
  bad "make-project with the plain compiler (see $log)"
fi

# 3. CMake: the toolchain file alone, Ninja and Makefiles
for gen in Ninja "Unix Makefiles"; do
  tag=$(echo "$gen" | tr -d ' ' | tr 'A-Z' 'a-z')
  b=$work/cmake-$tag
  what="CMake ($gen), toolchain file only"
  if x cmake -S "$src/cmake-project" -B "$b/build" -G "$gen" -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
       -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$b/install" &&
     x cmake --build "$b/build" -j2 && x ctest --test-dir "$b/build" --output-on-failure &&
     x cmake --install "$b/build" && expect_output "$b/install/bin/app" "integration: ok" &&
     x cmake -S "$src/cmake-consumer" -B "$b/consumer" -G "$gen" -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
       -DCMAKE_PREFIX_PATH="$b/install" &&
     x cmake --build "$b/consumer" && expect_output "$b/consumer/consumer" "integration: ok"; then
    ok "$what: checks, FetchContent, ExternalProject, shared library, test, install, consumer"
    libycxx_only "$what" "$b/build" "$b/install" "$b/consumer/consumer"
  else
    bad "$what (see $log)"
  fi
done

# 4. CMake: CC and CXX in the environment (configure and build: the ExternalProject's configure
# runs at build time)
b=$work/cmake-env
if x env CXX="$prefix/bin/ycxx-c++" CC="$prefix/bin/ycxx-cc" cmake -S "$src/cmake-project" -B "$b" -G Ninja -DCMAKE_BUILD_TYPE=Release &&
   x env CXX="$prefix/bin/ycxx-c++" CC="$prefix/bin/ycxx-cc" cmake --build "$b" -j2 && expect_output "$b/app" "integration: ok"; then
  ok "CMake, CC/CXX in the environment: build and run"
  libycxx_only "CMake, CC/CXX in the environment" "$b"
else
  bad "CMake, CC/CXX in the environment (see $log)"
fi

# 5. make: the wrapper as CXX, and the plain compiler with pkg-config
b=$work/make-wrapper
mkdir -p "$b"
if x make -C "$b" -f "$src/make-project/Makefile" VPATH="$src/make-project" CXX="$prefix/bin/ycxx-c++" CC="$prefix/bin/ycxx-cc" &&
   expect_output "$b/app" "make ok: 42"; then
  ok "make, CXX=ycxx-c++: build and run"
  libycxx_only "make, CXX=ycxx-c++" "$b/app" "$b/main.o" "$b/util.o"
else
  bad "make, CXX=ycxx-c++ (see $log)"
fi
b=$work/make-pkgconfig
mkdir -p "$b"
if ! command -v pkg-config >/dev/null 2>&1; then
  ui_skip "[$c] make with pkg-config" "(no pkg-config)"
elif x env PKG_CONFIG_PATH="$prefix/lib/pkgconfig" make -C "$b" -f "$src/make-project/Makefile" VPATH="$src/make-project" \
       CXX="$cxx" CC="$cc" PKG=libycxx && expect_output "$b/app" "make ok: 42"; then
  ok "make, plain compiler and pkg-config's libycxx.pc: build and run"
  libycxx_only "make, pkg-config" "$b/app" "$b/main.o" "$b/util.o"
else
  bad "make with pkg-config (see $log)"
fi

# 6. a moved installation
moved=$work/moved-prefix
cp -R "$prefix" "$moved"
b=$work/make-moved
mkdir -p "$b"
if x env PKG_CONFIG_PATH="$moved/lib/pkgconfig" make -C "$b" -f "$src/make-project/Makefile" VPATH="$src/make-project" \
     CXX="$cxx" CC="$cc" PKG=libycxx &&
   expect_output "$b/app" "make ok: 42" && rm -f "$b"/app "$b"/*.o && x make -C "$b" -f "$src/make-project/Makefile" VPATH="$src/make-project" \
     CXX="$moved/bin/ycxx-c++" CC="$moved/bin/ycxx-cc" &&
   expect_output "$b/app" "make ok: 42" &&
   [ "$(PKG_CONFIG_PATH=$moved/lib/pkgconfig pkg-config --variable=includedir libycxx | sed 's|/lib/pkgconfig/\.\./\.\.||')" = "$moved/include/libycxx" ]; then
  ok "a moved installation: ycxx-c++ and libycxx.pc follow it"
else
  bad "a moved installation (see $log)"
fi
rm -rf "$moved"

# 7. activate.sh --use (bash), with a toolchains.env of its own (activation reads one)
if command -v bash >/dev/null 2>&1; then
  b=$work/activate
  mkdir -p "$b/toolchains"
  printf 'YCXX_GCC=%s\nYCXX_GXX=%s\n' "${YCXX_GCC:-gcc-16}" "${YCXX_GXX:-g++-16}" >"$b/toolchains/toolchains.env"
  if x env YCXX_TOOLCHAINS="$b/toolchains" bash -c '. "$1/tools/toolchain/activate.sh" --use "$2" &&
                [ "$CXX" = "$2/bin/ycxx-c++" ] && [ "$CMAKE_TOOLCHAIN_FILE" = "$2/lib/cmake/libycxx/toolchain.cmake" ] &&
                pkg-config --exists libycxx &&
                make -C "$3" -f "$1/tests/integration/make-project/Makefile" VPATH="$1/tests/integration/make-project" CXX="$CXX" CC="$CC" &&
                ycxx-unload && [ -z "${CMAKE_TOOLCHAIN_FILE:-}" ] && [ -z "${CXX:-}" ]' sh "$repo" "$prefix" "$b" &&
     expect_output "$b/app" "make ok: 42"; then
    ok "activate.sh --use: CXX, CC, CMAKE_TOOLCHAIN_FILE, PKG_CONFIG_PATH; ycxx-unload restores them"
  else
    bad "activate.sh --use (see $log)"
  fi
else
  ui_skip "[$c] activate.sh --use" "(no bash)"
fi

# 8. Meson
meson=
if command -v meson >/dev/null 2>&1; then meson=meson
elif command -v uvx >/dev/null 2>&1; then meson="uvx --quiet --from meson meson"; fi
if [ -n "$meson" ]; then
  b=$work/meson
  if x $meson setup "$b" "$src/meson-project" --native-file "$prefix/share/libycxx/meson-native.ini" &&
     x $meson compile -C "$b" && x $meson test -C "$b" && expect_output "$b/app" "meson ok"; then
    ok "Meson with the native file: build, test, run"
    libycxx_only "Meson" "$b"
  else
    bad "Meson with the native file (see $log)"
  fi
else
  ui_skip "[$c] Meson" "(neither meson nor uvx)"
fi

# 9. autotools
if command -v autoreconf >/dev/null 2>&1 && command -v automake >/dev/null 2>&1; then
  b=$work/autotools
  mkdir -p "$b/build"
  cp -R "$src/autotools-project" "$b/src"
  if (cd "$b/src" && x autoreconf -i) &&
     (cd "$b/build" && x ../src/configure CXX="$prefix/bin/ycxx-c++" CC="$prefix/bin/ycxx-cc" "CXXFLAGS=-O2 -std=c++17") &&
     x make -C "$b/build" check && expect_output "$b/build/app" "autotools ok"; then
    ok "autotools, ./configure CXX=ycxx-c++: configure checks, make check"
    libycxx_only "autotools" "$b/build/app"
  else
    bad "autotools (see $log)"
  fi
else
  ui_skip "[$c] autotools" "(no autoreconf/automake)"
fi

exit $fail
