#!/bin/sh
# The Linux feasibility probes of the shared-library design (DECISIONS §20, "Probes").
#
#   docs/design/shared-probes/run-linux.sh [gcc] [clang]      (default: both)
#
# Work directory: $PROBE_WORK (default: a new directory under ${TMPDIR:-/tmp}). Needs g++-16 and
# clang++-23 ($YCXX_GXX, $YCXX_CLANGXX), cmake, ninja, patch, and for the libc++ cases libc++ for
# Clang (clang++ -stdlib=libc++). Builds with -j2 ($PROBE_JOBS).
#
# 1. Trees. Two copies of the repository's HEAD: plain-std.patch splits off the blocks that stay in
#    plain `std` (DECISIONS §20.5), then transform.py opens `std { inline namespace __y1 {`
#    everywhere else:
#      y1-static   hidden, as today (DECISIONS §2)
#      y1-shared   default visibility on the std and __ycxx blocks (the ABI entry points, __cxxabiv1,
#                  the PAL and the plain-std blocks stay hidden)
#    and builds libycxx's archives from each, per compiler.
# 2. libycxx.so.0.1: y1-shared's archives linked whole (-soname libycxx.so.0.1), and
#    libycxx_nonshared.a, the part every image links itself, hidden (DECISIONS §20.6). A libdir for
#    tools/ycxx-cxx holds libycxx.a as a GNU ld linker script, GROUP(libycxx.so.0.1 <archive>).
# 3. known/: the compiler-known entities, against y1-static and against y1-shared + libycxx.so.
# 4. coexist/: a shared-mode libycxx library (mine) and a library built with the toolchain's own
#    (other: libstdc++, or libc++) in one process, each throwing, catching, allocating and using
#    std::string/std::vector with its own runtime: a libycxx program, a toolchain program, and a C
#    host dlopen()ing both in both orders, RTLD_GLOBAL and RTLD_LOCAL.
# 5. plugin/: a host dlopen()s a plugin, exchanging std::string/std::vector and an exception, for
#    every pair of modes (static/shared host x static/shared plugin).
# 6. Exports: what libycxx.so and the probe images export.
# Prints PASS/FAIL per check and a summary; exit status 1 when any check failed.
set -u
here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../../.." && pwd)
compilers=${*:-gcc clang}
work=${PROBE_WORK:-$(mktemp -d "${TMPDIR:-/tmp}/ycxx-shared-probes.XXXXXX")}
jobs=${PROBE_JOBS:-2}
gxx=${YCXX_GXX:-g++-16}
clangxx=${YCXX_CLANGXX:-clang++-23}
mkdir -p "$work"
log=$work/run.log
: >"$log"
pass=0 fail=0
ok() { echo "PASS $*"; pass=$((pass + 1)); }
bad() { echo "FAIL $*"; fail=$((fail + 1)); }
run() { echo "\$ $*" >>"$log"; "$@" >>"$log" 2>&1; }
echo "work directory: $work (commands and output: $log)"

# ---- 1. trees ----
for m in static shared; do
  t=$work/y1-$m
  if [ ! -f "$t/.transformed" ]; then
    rm -rf "$t"; mkdir -p "$t"
    git -C "$repo" archive HEAD include src modules cmake tools CMakeLists.txt tests/ycxx tests/ycxxlit tests/common |
      tar -x -C "$t" || exit 1
    run patch -p1 -d "$t" -i "$here/plain-std.patch" || { echo "plain-std.patch does not apply"; exit 1; }
    run python3 "$here/transform.py" "$t" $m || exit 1
    touch "$t/.transformed"
  fi
  for cc in $compilers; do
    case $cc in gcc) cxx=$gxx cc_c=${YCXX_GCC:-gcc-16} ;; clang) cxx=$clangxx cc_c=${YCXX_CLANG:-clang-23} ;; esac
    b=$t/build/$cc
    if [ ! -f "$b/libycxx.a" ]; then
      run cmake -S "$t" -B "$b" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=$(command -v $cxx) \
        -DCMAKE_C_COMPILER=$(command -v $cc_c) -DYCXX_MODULES=OFF -DYCXX_WERROR=OFF -DYCXX_INSTALL=OFF &&
        run ninja -C "$b" -j"$jobs"
    fi
    [ -f "$b/libycxx.a" ] && ok "[$cc] build $m tree" || bad "[$cc] build $m tree"
  done
done

# ---- 2. libycxx.so.0.1, the per-image archive and the libdirs ----
# PROBE_VARIANT=nonshared (default): the design's split (§5). libycxx.so.0.1 holds the one ABI runtime
# and exports its entry points as __ycxx_abi_<name> (nonshared/export.c here); libycxx_nonshared.a gives
# each image hidden forwarders under the names the compilers call (nonshared/forward.c), the
# __cxxabiv1 vtables and fundamental type_info objects (nonshared/rtti.cpp), std::nothrow, and
# the default allocation functions with the allocation table (y1-shared's archive members, whose
# own declarations transform.py keeps hidden: per image, as today). PROBE_VARIANT=perimage: each image links y1-static's whole runtime after libycxx.so.0.1
# instead, so its ABI runtime is its own (the model of DECISIONS §2 kept for the ABI part).
variant=${PROBE_VARIANT:-nonshared}
for cc in $compilers; do
  case $cc in gcc) cxx=$gxx cc_c=${YCXX_GCC:-gcc-16} ;; clang) cxx=$clangxx cc_c=${YCXX_CLANG:-clang-23} ;; esac
  sh=$work/y1-shared/build/$cc st=$work/y1-static/build/$cc so=$work/so-$cc
  rm -rf "$so"; mkdir -p "$so/ns"
  extra=
  [ $cc = clang ] && extra="--gcc-install-dir=$(dirname "$($gxx -print-libgcc-file-name)")"
  python3 "$here/nonshared/gen_forward.py" "$so/ns"
  exports=
  if [ $variant = nonshared ]; then
    run $cc_c -O2 -fPIC -funwind-tables -c "$so/ns/export.c" -o "$so/export.o" && exports=$so/export.o
  fi
  run $cxx $extra -shared -o "$so/libycxx.so.0.1" -Wl,-soname,libycxx.so.0.1 $exports \
    -Wl,--whole-archive "$sh/libycxx.a" "$sh/libycxx-abi.a" -Wl,--no-whole-archive -nostdlib++ -lm -shared-libgcc &&
    ln -sf libycxx.so.0.1 "$so/libycxx.so" && ok "[$cc] link libycxx.so.0.1 ($variant)" || bad "[$cc] link libycxx.so.0.1 ($variant)"
  # libycxx_nonshared.a
  (cd "$so/ns" && ar x "$sh/libycxx.a" $(ar t "$sh/libycxx.a" | grep -E '^(new|delete)[a-z_]*\.cpp\.o$|^allocation_table\.cpp\.o$')) &&
    (cd "$so/ns" && ar x "$sh/libycxx-abi.a" rtti_float16.cpp.o) &&
  run $cc_c -O2 -fPIC -funwind-tables -c "$so/ns/forward.c" -o "$so/ns/forward.o" &&
  run "$work/y1-shared/tools/ycxx-cxx" $cc --libdir="$sh" -O2 -fPIC -frtti -I"$work/y1-shared/src/abi" -I"$sh/generated" \
    -c "$here/nonshared/rtti.cpp" -o "$so/ns/rtti.o" &&
  run ar rcs "$so/libycxx_nonshared.a" "$so"/ns/*.o && ok "[$cc] build libycxx_nonshared.a" || bad "[$cc] build libycxx_nonshared.a"
  # tools/ycxx-cxx --libdir: libycxx.a is a linker script, as glibc's libc.so is.
  if [ $variant = nonshared ]; then
    printf 'GROUP(%s %s)\n' "$so/libycxx.so.0.1" "$so/libycxx_nonshared.a" >"$so/libycxx.a"
  else
    printf 'GROUP(%s %s %s)\n' "$so/libycxx.so.0.1" "$st/libycxx-abi.a" "$st/libycxx.a" >"$so/libycxx.a"
  fi
  printf '/* empty: libycxx.a names everything */\n' >"$so/libycxx-abi.a"
  ln -s "$sh/generated" "$so/generated"
  cp "$sh/ycxx-link-options" "$so/ycxx-link-options"
  n=$(nm -D --defined-only "$so/libycxx.so.0.1" | wc -l)
  foreign=$(nm -D --defined-only "$so/libycxx.so.0.1" | awk '{print $NF}' | grep -vE 'St4__y1|6__ycxx|^__ycxx_allocation_functions$|^__ycxx_abi_' | tr '\n' ' ')
  [ -z "$foreign" ] && ok "[$cc] libycxx.so.0.1 exports $n symbols, all std::__y1, __ycxx, __ycxx_abi_* or the allocation table" ||
    bad "[$cc] libycxx.so.0.1 also exports: $foreign"
done
export LD_LIBRARY_PATH="$work/so-gcc:$work/so-clang${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# ---- 3. the compiler-known entities ----
for cc in $compilers; do
  for m in static shared; do
    if [ $m = static ]; then libdir=$work/y1-static/build/$cc; else libdir=$work/so-$cc; fi
    YCXX_LIBDIR=$libdir PROBE_OUT=$work/known-$m PROBE_MODULES=1 sh "$here/run-known.sh" "$work/y1-$m" $cc \
      >"$work/known-$m-$cc.txt" 2>&1
    while IFS= read -r line; do
      case $line in
        *dangling_warning:\ COMPILE\ FAILED*dangling*) ok "$m $line" ;;  # the diagnostic is the expected result
        *": n/a"*) ;;
        *"run: ok"*) ok "$m $line" ;;
        *) bad "$m $line" ;;
      esac
    done <"$work/known-$m-$cc.txt"
  done
done

# ---- 4. coexistence with the toolchain's library ----
for cc in $compilers; do
  d=$work/coexist-$cc
  mkdir -p "$d"
  ycxx="$work/y1-shared/tools/ycxx-cxx $cc --libdir=$work/so-$cc"
  run $ycxx -O2 -fPIC -shared "$here/coexist/mine.cpp" -o "$d/libmine.so" || bad "[$cc] build libmine.so"
  # The other libraries: libstdc++ (g++), and libc++ (clang++ -stdlib=libc++) when Clang is probed.
  others=libstdcxx
  [ $cc = clang ] && others="libstdcxx libcxx"
  for o in $others; do
    case $o in
      libstdcxx) ocxx=$gxx ;;
      libcxx) ocxx="$clangxx -stdlib=libc++" ;;
    esac
    ldir=$(dirname "$($ocxx -print-file-name=$( [ $o = libcxx ] && echo libc++.so || echo libstdc++.so))")
    run $ocxx -std=c++20 -O2 -fPIC -shared "$here/coexist/other.cpp" -o "$d/libother-$o.so" -Wl,-rpath,"$ldir" ||
      bad "[$cc] build libother-$o.so"
    # A libycxx program (shared mode) linked with the other library.
    run $ycxx -O2 "$here/coexist/main.cpp" "$here/coexist/mine.cpp" -o "$d/prog-$o" "$d/libother-$o.so" -Wl,-rpath,"$d"
    out=$("$d/prog-$o" 2>&1); [ "$out" = "mine 31 other 31" ] && ok "[$cc/$o] libycxx program + $o library: $out" ||
      bad "[$cc/$o] libycxx program + $o library: $out"
    # A program built with the other library, linked with both libraries (both orders).
    run $ocxx -std=c++20 -O2 "$here/coexist/main.cpp" -o "$d/host-$o" "$d/libother-$o.so" "$d/libmine.so" -Wl,-rpath,"$d" -Wl,-rpath,"$ldir"
    out=$("$d/host-$o" 2>&1); [ "$out" = "mine 31 other 31" ] && ok "[$cc/$o] $o program + both libraries: $out" ||
      bad "[$cc/$o] $o program + both libraries: $out"
    run $ocxx -std=c++20 -O2 "$here/coexist/main.cpp" -o "$d/host2-$o" "$d/libmine.so" "$d/libother-$o.so" -Wl,-rpath,"$d" -Wl,-rpath,"$ldir"
    out=$("$d/host2-$o" 2>&1); [ "$out" = "mine 31 other 31" ] && ok "[$cc/$o] $o program + both libraries (libycxx first): $out" ||
      bad "[$cc/$o] $o program + both libraries (libycxx first): $out"
    # A C host dlopen()ing both, both orders, global and local.
    run ${YCXX_CC:-cc} -O2 "$here/coexist/dl.c" -o "$d/dl" -ldl
    for mode in global local; do
      for order in "libmine.so libother-$o.so" "libother-$o.so libmine.so"; do
        set -- $order
        out=$(cd "$d" && DL_MODE=$mode LD_LIBRARY_PATH="$d:$ldir:$LD_LIBRARY_PATH" ./dl "./$1" "./$2" 2>&1)
        [ "$out" = "mine 31 other 31" ] && ok "[$cc/$o] dlopen $1 then $2 ($mode): $out" ||
          bad "[$cc/$o] dlopen $1 then $2 ($mode): $out"
      done
    done
  done
done

# ---- 5. plugins across modes ----
for cc in $compilers; do
  d=$work/plugin-$cc
  mkdir -p "$d"
  for m in static shared; do
    if [ $m = static ]; then libdir=$work/y1-static/build/$cc; else libdir=$work/so-$cc; fi
    ycxx="$work/y1-$m/tools/ycxx-cxx $cc --libdir=$libdir"
    attr=
    [ $m = static ] && attr=-DPLUGIN_EXPORT_ATTRIBUTE  # GCC hides the functions otherwise (DECISIONS §2)
    run $ycxx -O2 -fPIC -shared $attr "$here/plugin/plugin.cpp" -o "$d/plugin-$m.so" ||
      bad "[$cc] build the $m plugin"
    run $ycxx -O2 $attr "$here/plugin/host.cpp" -o "$d/host-$m" -ldl || bad "[$cc] build the $m host"
    exported=$(nm -D --defined-only "$d/plugin-$m.so" | grep -c ' T plugin_')
    [ "$exported" = 7 ] && ok "[$cc] $m plugin exports its 7 functions${attr:+ (with the attribute)}" ||
      bad "[$cc] $m plugin exports $exported of its 7 functions"
  done
  for hm in static shared; do
    for pm in static shared; do
      out=$("$d/host-$hm" "$d/plugin-$pm.so" 2>&1)
      # One runtime per process (shared-state 15) is expected only when both use libycxx.so.
      want="plugin 15 shared-state 0"
      [ $hm$pm = sharedshared ] && want="plugin 15 shared-state 15"
      [ "$out" = "$want" ] && ok "[$cc] $hm host + $pm plugin: $out" || bad "[$cc] $hm host + $pm plugin: $out (expected $want)"
    done
  done
  # Shared mode without the attribute: GCC no longer hides a function whose signature names a
  # library type (STATUS, known limitations).
  run $work/y1-shared/tools/ycxx-cxx $cc --libdir=$work/so-$cc -O2 -fPIC -shared "$here/plugin/plugin.cpp" -o "$d/plugin-noattr.so"
  exported=$(nm -D --defined-only "$d/plugin-noattr.so" | grep -c ' T plugin_')
  [ "$exported" = 7 ] && ok "[$cc] shared plugin without the export attribute exports its 7 functions" ||
    bad "[$cc] shared plugin without the export attribute exports $exported of its 7 functions"
  run $work/y1-static/tools/ycxx-cxx $cc --libdir=$work/y1-static/build/$cc -O2 -fPIC -shared "$here/plugin/plugin.cpp" -o "$d/plugin-static-noattr.so"
  echo "[$cc] static plugin without the export attribute exports $(nm -D --defined-only "$d/plugin-static-noattr.so" | grep -c ' T plugin_') of its 7 functions (GCC: 1, plugin_uncaught, whose signature names no library type; Clang: all)"
  # What the images export besides their own functions: the shared plugin exports the inline
  # functions and template instantiations it emitted (std::__y1, default visibility), the static
  # one only the allocation table.
  for m in static shared; do
    echo "[$cc] $m plugin exports $(nm -D --defined-only "$d/plugin-$m.so" | wc -l) symbols, $(nm -D --defined-only "$d/plugin-$m.so" | grep -c 'St4__y1') of them std::__y1"
  done
done

echo "passed $pass, failed $fail (log: $log)"
[ $fail = 0 ]
