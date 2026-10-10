#!/bin/sh
# The shared library on a Mac (DECISIONS §20; written for macOS, where nothing else exercises dyld,
# and runnable on Linux too): builds libycxx with both kinds (-DYCXX_SHARED=ON) for each compiler,
# installs it into a scratch prefix, and checks
#   load       a program linked with YCXX_LINKAGE=shared ycxx-c++ loads libycxx.0.1.dylib
#              (@rpath) and runs examples/demo.cpp
#   name       the install name @rpath/libycxx.0.1.dylib and compatibility version 0.1 (ELF: the
#              soname libycxx.so.0.1)
#   exports    the library exports only std::__y1, __ycxx, __ycxx_abi_*, the allocation table and
#              the shared marker; its weak exports only std::__y1, __ycxx and the table; nothing of
#              libgcc's emulated TLS (___emutls_*: GCC's -static-libgcc, DECISIONS §20.12)
#   coexist    a shared-mode libycxx library next to one built with Apple's libc++ (and, with GCC,
#              Homebrew's libstdc++; on Linux the toolchain's libstdc++, and libc++ when Clang has
#              it): a libycxx program linked with the other library, a program of the other library
#              linked with both in both orders, and a C host dlopen()ing both in both orders with
#              RTLD_GLOBAL and RTLD_LOCAL; each library must keep its own runtime
#              ("mine 31 other 31"; the other side may score as in its control, the other library
#              alone in the C host: a libstdc++ dylib scores 27 there, DECISIONS §20.12)
#   plugins    hosts and plugins in both modes (tests/cmake/visibility/plugin): strings, vectors and
#              exceptions cross ("plugin 15"); the runtime state is one only between a shared host
#              and a shared plugin ("shared-state 15", else 0)
#   guard      a static-mode object does not link in shared mode, nor the reverse, and the error
#              names the missing marker (__ycxx_linkage_static_v1 / __ycxx_linkage_shared_v1)
#   skew       a plugin built against another libycxx ABI version aborts at load naming both
#              versions (the allocation table's check, DECISIONS §20.10 step 8)
#
#   tools/check-shared-macos.sh [gcc] [clang]      (default: both; a missing compiler is skipped)
#
# Compilers: $YCXX_GXX / $YCXX_CLANGXX (tools/toolchain/activate.sh), else g++-16 / clang++-23.
# Apple's libc++: $APPLE_CXX (default /usr/bin/clang++), the C host: $APPLE_CC (/usr/bin/clang).
# Work directory: $YCXX_SHARED_CHECK_DIR (default build/shared-check); builds use
# $CMAKE_BUILD_PARALLEL_LEVEL jobs (default 2). Needs cmake and ninja. Prints PASS, FAIL or INFO
# per check and a summary; exit status 1 when a check failed. Send the whole output, and
# <work>/log.txt when something failed.
set -u
repo=$(cd "$(dirname "$0")/.." && pwd)
. "$repo/tools/lib/env.sh"
ycxx_env_load
work=${YCXX_SHARED_CHECK_DIR:-$repo/build/shared-check}
mkdir -p "$work"
work=$(cd "$work" && pwd)
log=$work/log.txt
: >"$log"
CMAKE_BUILD_PARALLEL_LEVEL=${CMAKE_BUILD_PARALLEL_LEVEL:-2}
export CMAKE_BUILD_PARALLEL_LEVEL
compilers=${*:-gcc clang}
pass=0 fail=0
ok() { echo "PASS $*"; pass=$((pass + 1)); }
bad() { echo "FAIL $*"; fail=$((fail + 1)); }
info() { echo "INFO $*"; }
run() { printf '$ %s\n' "$*" >>"$log"; "$@" >>"$log" 2>&1; }

if [ "$(uname -s)" = Darwin ]; then
  darwin=1 dso=dylib shared_flag=-dynamiclib
  so_name=libycxx.0.1.dylib
else
  darwin=0 dso=so shared_flag=-shared
  so_name=libycxx.so.0.1
fi
# exports FILE: the defined, exported symbols (no leading '_' on Mach-O, no version on ELF).
exports() {
  if [ $darwin = 1 ]; then nm -gU "$1" | awk '{ print $NF }' | sed 's/^_//'
  else nm -D --defined-only "$1" | awk '{ print $NF }' | sed 's/@.*//'; fi
}
weak_exports() {
  if [ $darwin = 1 ]; then nm -m "$1" | grep 'weak external' | awk '{ print $NF }' | sed 's/^_//'
  else nm -D --defined-only "$1" | awk '$2 == "W" || $2 == "V" || $2 == "u" { print $NF }' | sed 's/@.*//'; fi
}
needs_shared() {
  if [ $darwin = 1 ]; then otool -L "$1" | grep -q '@rpath/libycxx\.0\.1\.dylib'
  else readelf -d "$1" | grep -q 'NEEDED.*\[libycxx\.so\.0\.1\]'; fi
}
dll() { # dll SRC OUT [flags]: a shared library (its install name: its path)
  dl_src=$1 dl_out=$2; shift 2
  if [ $darwin = 1 ]; then run "$@" -O2 -fPIC -dynamiclib "$dl_src" -o "$dl_out" -install_name "$dl_out"
  else run "$@" -O2 -fPIC -shared "$dl_src" -o "$dl_out"; fi
}

coexist=$repo/tests/cmake/visibility/coexist
plugin=$repo/tests/cmake/visibility/plugin
for c in $compilers; do
  if [ $c = gcc ]; then cc=${YCXX_GCC:-gcc-16} cxx=${YCXX_GXX:-g++-16}; else cc=${YCXX_CLANG:-clang-23} cxx=${YCXX_CLANGXX:-clang++-23}; fi
  if ! command -v "$cxx" >/dev/null 2>&1; then info "[$c] $cxx not found: skipped"; continue; fi
  d=$work/$c
  rm -rf "$d"
  mkdir -p "$d"
  p=$d/prefix
  echo "== $c ($cxx): building and installing libycxx with both kinds into $p"
  if run cmake -S "$repo" -B "$d/lib" -G Ninja -DCMAKE_C_COMPILER="$cc" -DCMAKE_CXX_COMPILER="$cxx" \
       -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$p" -DYCXX_SHARED=ON &&
     run cmake --build "$d/lib" && run cmake --install "$d/lib"; then
    ok "[$c] build and install both kinds"
  else
    bad "[$c] build and install (see $log)"; continue
  fi
  so=$p/lib/$so_name
  ycxx_static="env YCXX_LINKAGE=static $p/bin/ycxx-c++"
  ycxx_shared="env YCXX_LINKAGE=shared $p/bin/ycxx-c++"

  # load
  if run $ycxx_shared -O2 "$repo/examples/demo.cpp" -o "$d/demo" && out=$("$d/demo" 2>&1) &&
     [ "$out" = "libycxx example: ok" ] && needs_shared "$d/demo"; then
    ok "[$c] load: a shared-mode program loads $so_name and runs"
  else
    bad "[$c] load: the shared-mode program (see $log)"
  fi
  # name
  if [ $darwin = 1 ]; then
    if otool -D "$so" | grep -qx '@rpath/libycxx.0.1.dylib' && otool -L "$so" | grep -q 'compatibility version 0\.1\.0'; then
      ok "[$c] name: @rpath/libycxx.0.1.dylib, compatibility version 0.1.0"
    else otool -D -L "$so" >>"$log" 2>&1; bad "[$c] name: install name or compatibility version (see $log)"; fi
  else
    if readelf -d "$so" | grep -q 'SONAME.*\[libycxx\.so\.0\.1\]'; then ok "[$c] name: soname libycxx.so.0.1"
    else bad "[$c] name: soname"; fi
  fi
  # exports
  all=$(exports "$so")
  foreign=$(printf '%s\n' "$all" | grep -vE 'St4__y1|6__ycxx|^__ycxx_abi_|^__ycxx_allocation_functions$|^__ycxx_linkage_shared_v1$|^YCXX_' || :)
  weak=$(weak_exports "$so")
  weak_foreign=$(printf '%s\n' "$weak" | grep -vE '^$|St4__y1|6__ycxx|^__ycxx_allocation_functions$' || :)
  emutls=$(printf '%s\n' "$all" | grep -i emutls || :)
  n=$(printf '%s\n' "$all" | grep -c . || :)
  nw=$(printf '%s\n' "$weak" | grep -c . || :)
  if [ "$n" -gt 0 ] && [ -z "$foreign" ]; then ok "[$c] exports: $n symbols, all std::__y1, __ycxx, __ycxx_abi_*, the table or the marker"
  else printf 'not libycxx'"'"'s own:\n%s\n' "$foreign" >>"$log"; bad "[$c] exports: other names exported (see $log)"; fi
  if [ -z "$weak_foreign" ]; then ok "[$c] exports: $nw weak exports, all std::__y1, __ycxx or the table"
  else printf 'weak, not libycxx'"'"'s own:\n%s\n' "$weak_foreign" >>"$log"; bad "[$c] exports: other weak exports (see $log)"; fi
  if [ -z "$emutls" ]; then ok "[$c] exports: none of libgcc's emulated-TLS entry points"
  else bad "[$c] exports: $emutls"; fi

  # coexist
  cx=$d/coexist
  mkdir -p "$cx"
  dll "$coexist/mine.cpp" "$cx/libmine.$dso" $ycxx_shared || bad "[$c] coexist: build libmine"
  others=
  apple=${APPLE_CXX:-/usr/bin/clang++}
  if [ $darwin = 1 ]; then
    others=libcxx
    [ $c = gcc ] && others="libcxx libstdcxx"
  else
    others=libstdcxx
    if [ $c = clang ] && echo 'int main(){}' | $cxx -x c++ -stdlib=libc++ - -o "$cx/probe" >/dev/null 2>&1; then
      others="libstdcxx libcxx"
    fi
  fi
  ccc=${APPLE_CC:-/usr/bin/clang}
  [ $darwin = 1 ] || ccc=$cc
  dlflag=
  [ $darwin = 0 ] && dlflag=-ldl
  run $ccc -O2 "$coexist/dl.c" -o "$cx/dl" $dlflag || bad "[$c] coexist: build the C host"
  for o in $others; do
    if [ $o = libcxx ]; then
      if [ $darwin = 1 ]; then ocxx="$apple -stdlib=libc++"; else ocxx="$cxx -stdlib=libc++"; fi
    else
      ocxx=$cxx
    fi
    rp=
    # The toolchain's own library may not be the system's (GCC 16's libstdc++ under its prefix).
    lib=$($ocxx -print-file-name=libstdc++.$dso 2>/dev/null)
    [ $o = libcxx ] && lib=$($ocxx -print-file-name=libc++.$dso 2>/dev/null)
    case $lib in /*) rp=-Wl,-rpath,$(dirname "$lib") ;; esac
    dll "$coexist/other.cpp" "$cx/libother-$o.$dso" $ocxx -std=c++20 $rp || bad "[$c/$o] coexist: build libother"
    run $ycxx_shared -O2 "$coexist/main.cpp" "$coexist/mine.cpp" -o "$cx/prog-$o" "$cx/libother-$o.$dso" -Wl,-rpath,"$cx" $rp
    out=$("$cx/prog-$o" 2>&1)
    [ "$out" = "mine 31 other 31" ] && ok "[$c/$o] coexist: libycxx program + $o library: $out" || bad "[$c/$o] coexist: libycxx program + $o library: $out"
    for order in "libother-$o.$dso libmine.$dso" "libmine.$dso libother-$o.$dso"; do
      set -- $order
      run $ocxx -std=c++20 -O2 "$coexist/main.cpp" -o "$cx/host-$o-$1" "$cx/$1" "$cx/$2" -Wl,-rpath,"$cx" $rp
      out=$(LD_LIBRARY_PATH=$p/lib "$cx/host-$o-$1" 2>&1)
      [ "$out" = "mine 31 other 31" ] && ok "[$c/$o] coexist: $o program + $1 then $2: $out" ||
        bad "[$c/$o] coexist: $o program + $1 then $2: $out"
    done
    control=$(DL_MODE=global "$cx/dl" "$cx/libother-$o.$dso" 2>&1 | sed -n 's/.*other \([-0-9]*\)$/\1/p')
    [ "$control" = 31 ] || info "[$c/$o] coexist: the control, libother-$o alone in a C host, scores $control without libycxx"
    for mode in global local; do
      for order in "libmine.$dso libother-$o.$dso" "libother-$o.$dso libmine.$dso"; do
        set -- $order
        out=$(DL_MODE=$mode "$cx/dl" "$cx/$1" "$cx/$2" 2>&1)
        if [ "$out" = "mine 31 other 31" ]; then ok "[$c/$o] coexist: dlopen $1 then $2 ($mode): $out"
        elif [ "$out" = "mine 31 other $control" ]; then ok "[$c/$o] coexist: dlopen $1 then $2 ($mode): $out (as the control)"
        else bad "[$c/$o] coexist: dlopen $1 then $2 ($mode): $out (control: other $control)"; fi
      done
    done
  done

  # plugins
  pl=$d/plugins
  mkdir -p "$pl"
  for m in static shared; do
    y=$ycxx_static; attr=-DPLUGIN_EXPORT_ATTRIBUTE
    [ $m = shared ] && y=$ycxx_shared attr=
    dll "$plugin/plugin.cpp" "$pl/plugin-$m.$dso" $y $attr || bad "[$c] plugins: build the $m plugin"
    dl=; [ $darwin = 0 ] && dl=-ldl
    run $y -O2 "$plugin/host.cpp" -o "$pl/host-$m" $dl || bad "[$c] plugins: build the $m host"
    info "[$c] plugins: the $m plugin exports $(exports "$pl/plugin-$m.$dso" | grep -c . || :) symbols, $(exports "$pl/plugin-$m.$dso" | grep -c 'St4__y1' || :) of them std::__y1"
  done
  for hm in static shared; do
    for pm in static shared; do
      want="plugin 15 shared-state 0"
      [ $hm$pm = sharedshared ] && want="plugin 15 shared-state 15"
      out=$("$pl/host-$hm" "$pl/plugin-$pm.$dso" 2>&1)
      [ "$out" = "$want" ] && ok "[$c] plugins: $hm host + $pm plugin: $out" || bad "[$c] plugins: $hm host + $pm plugin: $out (expected $want)"
    done
  done

  # skew
  dlflag=; [ $darwin = 0 ] && dlflag="-ldl -Wl,--export-dynamic-symbol=__ycxx_allocation_functions"
  if run $ccc -O2 "$repo/tests/cmake/visibility/skew.c" -o "$d/skew" $dlflag; then
    for pm in static shared; do
      out=$("$d/skew" "$pl/plugin-$pm.$dso" 2>&1) && st=0 || st=$?
      printf '$ %s %s\n%s\n[exit %s]\n' "$d/skew" "$pl/plugin-$pm.$dso" "$out" "$st" >>"$log"
      if [ "$st" != 0 ] && printf '%s\n' "$out" | grep -q 'built against libycxx 0\.1, the process.s first libycxx image against libycxx 0\.0-skewed'; then
        ok "[$c] skew: a $pm plugin stops in a process of another libycxx version, naming both"
      else
        bad "[$c] skew: the $pm plugin did not stop with the message: $out"
      fi
    done
  else
    bad "[$c] skew: building the host (see $log)"
  fi

  # guard
  if run $ycxx_static -c "$repo/examples/demo.cpp" -o "$d/static.o" && run $ycxx_shared -c "$repo/examples/demo.cpp" -o "$d/shared.o"; then
    $ycxx_shared "$d/static.o" -o "$d/mismatch1" >"$d/mismatch1.log" 2>&1 && m1=linked || m1=failed
    $ycxx_static "$d/shared.o" -o "$d/mismatch2" >"$d/mismatch2.log" 2>&1 && m2=linked || m2=failed
    if [ $m1 = failed ] && grep -q '__ycxx_linkage_static_v1' "$d/mismatch1.log"; then
      ok "[$c] guard: a static-mode object does not link in shared mode (names __ycxx_linkage_static_v1)"
    else bad "[$c] guard: static-mode object in a shared-mode link: $m1 (see $d/mismatch1.log)"; fi
    if [ $m2 = failed ] && grep -q '__ycxx_linkage_shared_v1' "$d/mismatch2.log"; then
      ok "[$c] guard: a shared-mode object does not link in static mode (names __ycxx_linkage_shared_v1)"
    else bad "[$c] guard: shared-mode object in a static-mode link: $m2 (see $d/mismatch2.log)"; fi
  else
    bad "[$c] guard: compiling (see $log)"
  fi
  info "[$c] a shared-mode program exports $(exports "$d/demo" | grep -c . || :) symbols, $(weak_exports "$d/demo" | grep -c . || :) weak"
done

echo "passed $pass, failed $fail (log: $log)"
[ $fail = 0 ]
