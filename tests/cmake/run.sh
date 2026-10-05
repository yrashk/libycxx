#!/bin/sh
# Tests libycxx's CMake support with the projects in examples/, for GCC and Clang:
#   1. configure, build and install libycxx into a scratch prefix;
#   2. examples/find_package: find_package(libycxx) from that prefix, build, run;
#   3. examples/add_subdirectory: libycxx built as part of the project, build, run;
#   4. both programs must print "libycxx example: ok" and must not use the toolchain's C++
#      library (no libstdc++/libc++ in NEEDED, no libstdc++ symbol versions);
#   5. find_package must reject an unsupported compiler with a clear message;
#   6. cmake/ycxx-toolchain.cmake: picks GCC and Clang by itself (with a scratch toolchain cache,
#      which it must fill in toolchains.env), and fails with a clear message when the requested
#      version is unavailable and YCXX_PROVISION is off. With YCXX_TEST_PROVISION=1 it also
#      downloads Clang into a scratch cache (about 2 GB).
#
#   tests/cmake/run.sh [gcc] [clang]        (default: both)
# Compilers come from the YCXX_* variables (tools/toolchain/activate.*), else g++-16 /
# clang++-23. Needs cmake and ninja.
set -eu
repo=$(cd "$(dirname "$0")/../.." && pwd)
. "$repo/tools/lib/ui.sh"
. "$repo/tools/lib/env.sh"
ycxx_env_load
work=${YCXX_CMAKE_TEST_DIR:-$repo/build/cmake-test}
compilers=${*:-gcc clang}
fail=0

ok() { ui_ok "[$1] $2"; }
bad() { ui_fail "[$1] $2"; fail=1; }
# x CMD...: show the command, run it with its output appended to $log.
x() { ui_cmd "$@"; "$@" >>"$log" 2>&1; }

# links_toolchain_cxx EXE: true if EXE depends on libstdc++ or libc++.
links_toolchain_cxx() {
  if [ "$(uname -s)" = Darwin ]; then
    otool -L "$1" | grep -E 'libstdc\+\+|libc\+\+' >/dev/null
  else
    readelf -d "$1" | grep -E 'NEEDED.*(libstdc\+\+|libc\+\+)' >/dev/null ||
      nm -D "$1" 2>/dev/null | grep -E 'GLIBCXX|CXXABI_1' >/dev/null
  fi
}

for c in $compilers; do
  case $c in
    gcc) cc=${YCXX_GCC:-gcc-16} cxx=${YCXX_GXX:-g++-16} ;;
    clang) cc=${YCXX_CLANG:-clang-23} cxx=${YCXX_CLANGXX:-clang++-23} ;;
    *) echo "unknown compiler $c" >&2; exit 2 ;;
  esac
  ui_section "CMake package with $c ($cxx)"
  d=$work/$c
  rm -rf "$d"
  mkdir -p "$d"
  log=$d/log.txt
  ui_info "log" "$log"
  gen="-G Ninja -DCMAKE_C_COMPILER=$cc -DCMAKE_CXX_COMPILER=$cxx -DCMAKE_BUILD_TYPE=Release"

  # 1. build and install
  if x cmake -S "$repo" -B "$d/lib" $gen -DCMAKE_INSTALL_PREFIX="$d/prefix" &&
     x cmake --build "$d/lib" &&
     x cmake --install "$d/lib"; then
    ok $c "build and install"
  else
    bad $c "build and install (see $log)"; continue
  fi
  for f in lib/libycxx.a lib/libycxx-abi.a include/libycxx/functional include/libycxx/ycxx/config.hpp \
           lib/cmake/libycxx/libycxxConfig.cmake lib/cmake/libycxx/libycxxConfigVersion.cmake; do
    [ -e "$d/prefix/$f" ] || bad $c "installed file missing: $f"
  done

  # 2, 3. the example projects
  for ex in find_package add_subdirectory; do
    b=$d/$ex
    if x cmake -S "$repo/examples/$ex" -B "$b" $gen -DCMAKE_PREFIX_PATH="$d/prefix" \
         -DLIBYCXX_SOURCE_DIR="$repo" &&
       x cmake --build "$b"; then
      out=$("$b/demo" 2>&1) || true
      if [ "$out" = "libycxx example: ok" ]; then ok $c "$ex: build and run"
      else bad $c "$ex: unexpected output: $out"; fi
      # 4. no toolchain C++ library
      if links_toolchain_cxx "$b/demo"; then
        bad $c "$ex: links the toolchain's C++ library"
      else
        ok $c "$ex: no libstdc++/libc++"
      fi
    else
      bad $c "$ex: configure/build (see $log)"
    fi
  done
done

# 5. an unsupported compiler is rejected by find_package (any installed prefix will do)
for c in $compilers; do
  prefix=$work/$c/prefix
  [ -d "$prefix" ] || continue
  old=$work/old-compiler
  rm -rf "$old"
  if command -v gcc >/dev/null && [ "$(gcc -dumpversion | cut -d. -f1)" -lt 16 ] 2>/dev/null; then
    if cmake -S "$repo/examples/find_package" -B "$old" -G Ninja -DCMAKE_CXX_COMPILER=g++ \
         -DCMAKE_PREFIX_PATH="$prefix" >"$work/old.log" 2>&1; then
      bad "$c" "find_package accepted $(gcc -dumpversion)"
    elif grep -q "libycxx needs GCC >= 16.2 or Clang >= 23" "$work/old.log"; then
      ok "$c" "find_package rejects GCC $(gcc -dumpversion)"
    else
      bad "$c" "find_package failed without the expected message (see $work/old.log)"
    fi
  fi
  break
done

# 6. the toolchain file
ui_section "Toolchain file (cmake/ycxx-toolchain.cmake)"
for c in $compilers; do
  cache=$work/toolchain-cache-$c
  rm -rf "$cache" "$work/tc-$c"
  if YCXX_TOOLCHAINS=$cache cmake -S "$repo/examples/add_subdirectory" -B "$work/tc-$c" -G Ninja \
       -DCMAKE_TOOLCHAIN_FILE="$repo/cmake/ycxx-toolchain.cmake" -DYCXX_COMPILER=$c \
       -DLIBYCXX_SOURCE_DIR="$repo" >"$work/tc-$c.log" 2>&1 &&
     cmake --build "$work/tc-$c" >>"$work/tc-$c.log" 2>&1 &&
     [ "$("$work/tc-$c/demo")" = "libycxx example: ok" ]; then
    ok $c "toolchain file: compiler found, example built and run"
  else
    bad $c "toolchain file (see $work/tc-$c.log)"
  fi
  key=YCXX_GXX; [ $c = clang ] && key=YCXX_CLANGXX
  if grep -q "^$key=/" "$cache/toolchains.env" 2>/dev/null; then ok $c "toolchain file: cache written"
  else bad $c "toolchain file: $key missing from $cache/toolchains.env"; fi
done
rm -rf "$work/tc-missing"
if YCXX_TOOLCHAINS=$work/toolchain-cache-missing cmake -S "$repo/examples/add_subdirectory" -B "$work/tc-missing" \
     -G Ninja -DCMAKE_TOOLCHAIN_FILE="$repo/cmake/ycxx-toolchain.cmake" -DYCXX_COMPILER=gcc \
     -DYCXX_GCC_VERSION=99.1.0 >"$work/tc-missing.log" 2>&1; then
  bad toolchain "accepted a missing GCC 99.1"
elif grep -q "Configure with -DYCXX_PROVISION=ON" "$work/tc-missing.log"; then
  ok toolchain "missing version without YCXX_PROVISION fails clearly"
else
  bad toolchain "missing version failed without the expected message (see $work/tc-missing.log)"
fi
if [ "${YCXX_TEST_PROVISION:-0}" = 1 ]; then
  cache=$work/toolchain-cache-download
  rm -rf "$cache" "$work/tc-download"
  if YCXX_TOOLCHAINS=$cache cmake -S "$repo/examples/add_subdirectory" -B "$work/tc-download" -G Ninja \
       -DCMAKE_TOOLCHAIN_FILE="$repo/cmake/ycxx-toolchain.cmake" -DYCXX_COMPILER=clang \
       -DYCXX_LLVM_VERSION=${YCXX_TEST_LLVM_VERSION:-23.1.2} -DYCXX_PROVISION=ON \
       -DYCXX_USE_SYSTEM_COMPILERS=OFF \
       -DLIBYCXX_SOURCE_DIR="$repo" >"$work/tc-download.log" 2>&1 &&
     grep -q "^YCXX_CLANGXX=$cache/llvm-" "$cache/toolchains.env" &&
     cmake --build "$work/tc-download" >>"$work/tc-download.log" 2>&1 &&
     [ "$("$work/tc-download/demo")" = "libycxx example: ok" ]; then
    ok toolchain "YCXX_PROVISION=ON downloaded Clang into the cache and built the example"
  else
    bad toolchain "provisioning download (see $work/tc-download.log)"
  fi
  rm -rf "$cache"
fi

echo
if [ $fail = 0 ]; then ui_ok "CMake support: all checks passed"; else ui_fail "CMake support: failures (logs under $work)"; fi
exit $fail
