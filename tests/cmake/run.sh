#!/bin/sh
# Tests libycxx's CMake support with the projects in examples/, for GCC and Clang:
#   1. configure, build and install libycxx into a scratch prefix;
#   2. examples/find_package: find_package(libycxx) from that prefix, build, run;
#   3. examples/add_subdirectory: libycxx built as part of the project, build, run;
#   4. both programs must print "libycxx example: ok" and must not use the toolchain's C++
#      library (no libstdc++/libc++ in NEEDED, no libstdc++ symbol versions);
#   5. find_package must reject an unsupported compiler with a clear message.
#
#   tests/cmake/run.sh [gcc] [clang]        (default: both)
# Compilers come from the YCXX_* variables (tools/toolchain/activate.*), else g++-16 /
# clang++-23. Needs cmake and ninja.
set -eu
repo=$(cd "$(dirname "$0")/../.." && pwd)
work=${YCXX_CMAKE_TEST_DIR:-$repo/build/cmake-test}
compilers=${*:-gcc clang}
fail=0

ok() { echo "ok   [$1] $2"; }
bad() { echo "FAIL [$1] $2"; fail=1; }

for c in $compilers; do
  case $c in
    gcc) cc=${YCXX_GCC:-gcc-16} cxx=${YCXX_GXX:-g++-16} ;;
    clang) cc=${YCXX_CLANG:-clang-23} cxx=${YCXX_CLANGXX:-clang++-23} ;;
    *) echo "unknown compiler $c" >&2; exit 2 ;;
  esac
  d=$work/$c
  rm -rf "$d"
  mkdir -p "$d"
  log=$d/log.txt
  gen="-G Ninja -DCMAKE_C_COMPILER=$cc -DCMAKE_CXX_COMPILER=$cxx -DCMAKE_BUILD_TYPE=Release"

  # 1. build and install
  if cmake -S "$repo" -B "$d/lib" $gen -DCMAKE_INSTALL_PREFIX="$d/prefix" >>"$log" 2>&1 &&
     cmake --build "$d/lib" >>"$log" 2>&1 &&
     cmake --install "$d/lib" >>"$log" 2>&1; then
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
    if cmake -S "$repo/examples/$ex" -B "$b" $gen -DCMAKE_PREFIX_PATH="$d/prefix" \
         -DLIBYCXX_SOURCE_DIR="$repo" >>"$log" 2>&1 &&
       cmake --build "$b" >>"$log" 2>&1; then
      out=$("$b/demo" 2>&1) || true
      if [ "$out" = "libycxx example: ok" ]; then ok $c "$ex: build and run"
      else bad $c "$ex: unexpected output: $out"; fi
      # 4. no toolchain C++ library
      if readelf -d "$b/demo" | grep -E 'NEEDED.*(libstdc\+\+|libc\+\+)' >/dev/null ||
         nm -D "$b/demo" 2>/dev/null | grep -E 'GLIBCXX|CXXABI_1' >/dev/null; then
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

exit $fail
