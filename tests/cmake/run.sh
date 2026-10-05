#!/bin/sh
# Tests libycxx's CMake support with the projects in examples/, for GCC and Clang:
#   1. configure, build and install libycxx into a scratch prefix, with the freestanding runtime
#      archive (YCXX_FREESTANDING_RUNTIME; checked in 9.);
#   2. examples/find_package: find_package(libycxx) from that prefix, build, run;
#   3. examples/add_subdirectory: libycxx built as part of the project, build, run;
#   4. both programs must print "libycxx example: ok" and must not use the toolchain's C++
#      library (no libstdc++/libc++ in NEEDED, no libstdc++ symbol versions), and must export
#      none of libycxx's symbols (DECISIONS §2);
#   7. tests/cmake/visibility: a program and a shared library built with libycxx, each in one
#      process with a shared library built with the toolchain's C++ library: each library must
#      handle its own exceptions with its own runtime, and libycxx's must export nothing of it;
#      and a program built with libycxx must catch a libycxx shared library's exceptions;
#   5. find_package must reject an unsupported compiler with a clear message;
#   6. cmake/ycxx-toolchain.cmake: picks GCC and Clang by itself (with a scratch toolchain cache,
#      which it must fill in toolchains.env), and fails with a clear message when the requested
#      version is unavailable and YCXX_PROVISION is off. With YCXX_TEST_PROVISION=1 it also
#      downloads Clang into a scratch cache (about 2 GB);
#   8. Linux, Clang: the example programs link the C runtime startup files and libgcc of GCC 16's
#      installation ($YCXX_GXX), which the package passes with --gcc-install-dir (link map);
#   9. the freestanding runtime archive is installed, exported as ycxx::freestanding, and links
#      the freestanding smoke program (tests/freestanding) with no C library.
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

# run_demo EXE LOG: runs an example program, recording in LOG the command, its output (stderr
# too: a crash or "terminating" message), its exit status and the libraries it links; true when
# it exits 0 printing just "libycxx example: ok".
run_demo() {
  printf '$ %s\n' "$1" >>"$2"
  d_st=0
  d_out=$("$1" 2>&1) || d_st=$?
  printf '%s\n[exit %s]\n' "$d_out" "$d_st" >>"$2"
  if [ "$(uname -s)" = Darwin ]; then otool -L "$1" >>"$2" 2>&1 || :
  else readelf -d "$1" 2>/dev/null | grep NEEDED >>"$2" || :; fi
  [ "$d_st" = 0 ] && [ "$d_out" = "libycxx example: ok" ]
}

# links_toolchain_cxx EXE: true if EXE depends on libstdc++ or libc++.
links_toolchain_cxx() {
  if [ "$(uname -s)" = Darwin ]; then
    otool -L "$1" | grep -E 'libstdc\+\+|libc\+\+' >/dev/null
  else
    readelf -d "$1" | grep -E 'NEEDED.*(libstdc\+\+|libc\+\+)' >/dev/null ||
      nm -D "$1" 2>/dev/null | grep -E 'GLIBCXX|CXXABI_1' >/dev/null
  fi
}

# library_exports FILE: prints the symbols of libycxx that FILE (a program or shared library whose
# own code defines only extern "C" functions) exports: every mangled C++ name, and the ABI
# runtime's and the platform layer's C names. ELF: the dynamic symbol table. Darwin: the exported
# symbols, and the weak-definition binds dyld would coalesce with another image's (dyld_info);
# there GCC's fundamental type_info objects keep default visibility, and are tolerated (benign:
# libc++abi exports the same objects).
library_exports() {
  if [ "$(uname -s)" = Darwin ]; then
    { nm -gU "$1" | awk '{ print $NF }'
      if command -v dyld_info >/dev/null; then
        dyld_info -fixups "$1" | grep 'weak-def-coalesce' | awk '{ print $NF }' | sed 's|.*/||'
      fi; } | sed 's/^_//' |
      grep -E '^(_Z|__cxa_|__gxx_personality|__dynamic_cast|ycxx_pal_)' |
      grep -vE '^_ZT[IS](P|PK)?([a-z]|D[A-Za-z][A-Za-z0-9_]*)$' || :
  else
    nm -D --defined-only "$1" | awk '{ print $NF }' |
      grep -E '^(_Z|__cxa_|__gxx_personality|__dynamic_cast|ycxx_pal_)' || :
  fi
}

# check_exports COMPILER LABEL FILE: no libycxx symbol exported from FILE (listed in $log otherwise).
check_exports() {
  e_list=$(library_exports "$3")
  if [ -z "$e_list" ]; then
    ok "$1" "$2: exports none of libycxx's symbols"
  else
    printf '%s exports:\n%s\n' "$3" "$e_list" >>"$log"
    bad "$1" "$2: exports $(printf '%s\n' "$e_list" | wc -l | tr -d ' ') of libycxx's symbols (see $log)"
  fi
}

# run_pair EXE LOG EXPECTED: runs a program of tests/cmake/visibility; true when it exits 0 printing
# EXPECTED.
run_pair() {
  printf '$ %s\n' "$1" >>"$2"
  p_st=0
  p_out=$("$1" 2>&1) || p_st=$?
  printf '%s\n[exit %s]\n' "$p_out" "$p_st" >>"$2"
  [ "$p_st" = 0 ] && [ "$p_out" = "$3" ]
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
  if x cmake -S "$repo" -B "$d/lib" $gen -DCMAKE_INSTALL_PREFIX="$d/prefix" -DYCXX_FREESTANDING_RUNTIME=ON &&
     x cmake --build "$d/lib" &&
     x cmake --install "$d/lib"; then
    ok $c "build and install"
  else
    bad $c "build and install (see $log)"; continue
  fi
  for f in lib/libycxx.a lib/libycxx-abi.a include/libycxx/functional include/libycxx/ycxx/config.hpp \
           lib/cmake/libycxx/libycxxConfig.cmake lib/cmake/libycxx/libycxxConfigVersion.cmake \
           lib/libycxx-freestanding.a; do
    [ -e "$d/prefix/$f" ] || bad $c "installed file missing: $f"
  done

  # 9. the freestanding runtime archive (YCXX_FREESTANDING_RUNTIME): exported as
  # ycxx::freestanding, and the freestanding smoke program links with it and no C library
  # (tests/freestanding; for the host's architecture, as tools/check_freestanding.sh does for
  # bare-metal targets; linked, not run: its entry point does not return).
  if grep -q 'ycxx::freestanding' "$d/prefix/lib/cmake/libycxx/libycxxTargets.cmake" 2>/dev/null; then
    ok $c "freestanding runtime: installed and exported as ycxx::freestanding"
  else
    bad $c "freestanding runtime: ycxx::freestanding not exported"
  fi
  if [ "$(uname -s)" = Linux ]; then
    fs="-std=c++26 -ffreestanding -nostdinc -nostdinc++ -isystem $d/prefix/include/libycxx -fno-exceptions -fno-rtti -O2"
    libgcc=; [ $c = gcc ] && libgcc=$($cc -print-libgcc-file-name)
    if x $cxx $fs -c "$repo/tests/freestanding/smoke.cpp" -o "$d/fs-smoke.o" &&
       x $cxx $fs -O0 -c "$repo/tests/freestanding/smoke_o0.cpp" -o "$d/fs-smoke_o0.o" &&
       x $cc -ffreestanding -nostdlib -O2 -c "$repo/tests/freestanding/rt.c" -o "$d/fs-rt.o" &&
       x $cc -static -nostdlib -e _start "$d/fs-smoke.o" "$d/fs-smoke_o0.o" "$d/fs-rt.o" \
         "$d/prefix/lib/libycxx-freestanding.a" $libgcc -o "$d/fs-smoke.elf"; then
      ok $c "freestanding runtime: the smoke program links with it and no C library"
    else
      bad $c "freestanding runtime: smoke link (see $log)"
    fi
  fi

  # 2, 3. the example projects (with a link map: which C runtime files and libgcc were linked)
  for ex in find_package add_subdirectory; do
    b=$d/$ex
    map=
    [ "$(uname -s)" = Linux ] && map="-DCMAKE_EXE_LINKER_FLAGS=-Wl,-Map,$b/demo.map"
    if x cmake -S "$repo/examples/$ex" -B "$b" $gen -DCMAKE_PREFIX_PATH="$d/prefix" \
         -DLIBYCXX_SOURCE_DIR="$repo" $map &&
       x cmake --build "$b"; then
      if run_demo "$b/demo" "$log"; then ok $c "$ex: build and run"
      else bad $c "$ex: the program failed (see $log)"; fi
      # 8. Linux, Clang: the package passes --gcc-install-dir, so the startup files and libgcc
      # are GCC 16's, not those of the GCC installation Clang would pick by itself.
      if [ $c = clang ] && [ -n "$map" ]; then
        gcc_dir=$(dirname "$(${YCXX_GXX:-g++-16} -print-libgcc-file-name)")
        if grep -q "^LOAD $gcc_dir/crtbegin" "$b/demo.map"; then
          ok $c "$ex: links GCC 16's startup files and libgcc ($gcc_dir)"
        else
          printf 'expected %s/crtbegin*.o in %s; loaded:\n' "$gcc_dir" "$b/demo.map" >>"$log"
          grep '^LOAD .*crtbegin' "$b/demo.map" >>"$log" || :
          bad $c "$ex: does not link GCC 16's startup files (see $log)"
        fi
      fi
      # 4. no toolchain C++ library
      if links_toolchain_cxx "$b/demo"; then
        bad $c "$ex: links the toolchain's C++ library"
      else
        ok $c "$ex: no libstdc++/libc++"
      fi
      check_exports $c "$ex" "$b/demo"
    else
      bad $c "$ex: configure/build (see $log)"
    fi
  done

  # 7. libycxx and the toolchain's C++ library in one process
  b=$d/visibility
  if x cmake -S "$repo/tests/cmake/visibility" -B "$b" $gen -DCMAKE_PREFIX_PATH="$d/prefix" &&
     x cmake --build "$b"; then
    for p in prog host; do
      if run_pair "$b/$p" "$log" "mine 3 other 3"; then ok $c "visibility: $p: each library uses its own runtime"
      else bad $c "visibility: $p: wrong exception handling (see $log)"; fi
    done
    if run_pair "$b/catcher" "$log" "caught 15 uncaught 0 0"; then
      ok $c "visibility: catcher: catches a libycxx shared library's exceptions"
    else bad $c "visibility: catcher: wrong exception handling across libycxx images (see $log)"; fi
    check_exports $c "visibility: prog" "$b/prog"
    for f in "$b"/libmine.*; do check_exports $c "visibility: shared library" "$f"; done
  else
    bad $c "visibility: configure/build (see $log)"
  fi
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
     cmake --build "$work/tc-$c" --verbose >>"$work/tc-$c.log" 2>&1 &&
     run_demo "$work/tc-$c/demo" "$work/tc-$c.log"; then
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
     cmake --build "$work/tc-download" --verbose >>"$work/tc-download.log" 2>&1 &&
     run_demo "$work/tc-download/demo" "$work/tc-download.log"; then
    ok toolchain "YCXX_PROVISION=ON downloaded Clang into the cache and built the example"
  else
    bad toolchain "provisioning download (see $work/tc-download.log)"
  fi
  rm -rf "$cache"
fi

echo
if [ $fail = 0 ]; then ui_ok "CMake support: all checks passed"; else ui_fail "CMake support: failures (logs under $work)"; fi
exit $fail
