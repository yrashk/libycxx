#!/bin/sh
# The macOS probes of the shared-library design (DECISIONS §20, "Questions only a
# macOS run answers"). Run from a libycxx checkout of the shared-lib-design branch on a Mac:
#
#   docs/design/shared-probes/run-macos.sh            all parts
#   docs/design/shared-probes/run-macos.sh 1 2        only parts 1 and 2 (3 builds libycxx twice
#                                                     per compiler, a few minutes each)
#
# Needs: python3, git, patch; part 3 also cmake and ninja. No network.
# Compilers: $YCXX_CLANGXX (Clang 23) and $YCXX_GXX (Homebrew GCC 16), as tools/toolchain/activate.sh
# sets them, else read from the toolchains.env that tools/toolchain/provision wrote, else
# clang++-23 / g++-16 on PATH; a missing one is skipped (Apple's clang is always used for part 2). Apple's libc++ is reached through $APPLE_CXX (default
# /usr/bin/clang++, Apple's clang with the SDK's libc++). Work directory: $PROBE_WORK (default: a new
# directory under $TMPDIR). Builds use -j2 ($PROBE_JOBS).
#
# What each check answers (the questions of DECISIONS §20.12, "Questions only the macOS run answers"):
#   1  minimal/: which entities the compilers need in plain std (as on Linux). One line per probe
#      and compiler; PASS = the plain-std control works, and the line shows what the std::__y1
#      form does (question 1).
#   2  darwin/: dyld's weak-definition coalescing against Apple's libc++abi, for
#        "control: plain std, default visibility"   the hazard of DECISIONS §2 reproduced (question 2;
#                                                   INFO if it does not reproduce)
#        "std::__y1, default visibility"            shared mode's exports are left alone (question 3)
#        "plain std, hidden"                        the plain-std entities are left alone (question 4)
#        "inline variable, two dylibs"              coalescing among libycxx images (question 5, INFO)
#   3  the whole library: run-linux.sh's steps 1-6 with Mach-O's tools, per compiler:
#        "build static/shared tree"                 the transformed sources build on Darwin
#        "libycxx.0.1.dylib exports ..."            only std::__y1, __ycxx, __ycxx_abi_*, the table;
#                                                   weak exports only std::__y1/__ycxx (question 6)
#        "@rpath/libycxx.0.1.dylib"                 install name and versions (question 10)
#        "static|shared <probe>: run: ok"           compiler-known battery; in shared mode through the
#                                                   nonshared forwarders and Darwin's unwinder (question 7)
#        "<x> program + <y> library: mine 31 other 31", "dlopen ..."
#                                                   coexistence with Apple's libc++ / Homebrew's
#                                                   libstdc++, all load orders (question 9)
#        "<mode> host + <mode> plugin: plugin 15 shared-state N"
#                                                   string/vector/exception exchange across modes;
#                                                   one runtime only for shared+shared (question 8)
#        INFO export counts of plugins and programs (question 11)
#
# Output: one line per check, "PASS", "FAIL" or "INFO" (an observation the design records either
# way), then a summary. Please send the whole output (and $PROBE_WORK/run.log when something fails).
set -u
here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../../.." && pwd)
# The compilers: the variables tools/toolchain/activate.sh exports when it was sourced; else the
# same toolchains.env it reads (tools/lib/env.sh, POSIX sh); else g++-16 / clang++-23 on PATH
# (Homebrew's bin directories added).
. "$repo/tools/lib/env.sh"
ycxx_env_load
for d in /opt/homebrew/bin /usr/local/bin; do
  case ":$PATH:" in *":$d:"*) ;; *) [ -d "$d" ] && PATH=$PATH:$d ;; esac
done
export PATH
parts=${*:-1 2 3}
work=${PROBE_WORK:-$(mktemp -d "${TMPDIR:-/tmp}/ycxx-shared-macos.XXXXXX")}
jobs=${PROBE_JOBS:-2}
apple=${APPLE_CXX:-/usr/bin/clang++}
mkdir -p "$work"
log=$work/run.log
: >"$log"
pass=0 fail=0
ok() { echo "PASS $*"; pass=$((pass + 1)); }
bad() { echo "FAIL $*"; fail=$((fail + 1)); }
info() { echo "INFO $*"; }
run() { echo "\$ $*" >>"$log"; "$@" >>"$log" 2>&1; }
has() { command -v "$1" >/dev/null 2>&1; }
compilers=
has "${YCXX_CLANGXX:-clang++-23}" && compilers="$compilers clang"
has "${YCXX_GXX:-g++-16}" && compilers="$compilers gcc"
cxx_of() { case $1 in clang) echo "${YCXX_CLANGXX:-clang++-23}" ;; gcc) echo "${YCXX_GXX:-g++-16}" ;; esac; }
cc_of() {
  case $1 in
    clang) c=${YCXX_CLANGXX:-clang++-23}; echo "${YCXX_CLANG:-$(dirname "$(command -v "$c")")/$(basename "$c" | sed 's/clang++/clang/')}" ;;
    gcc) c=${YCXX_GXX:-g++-16}; echo "${YCXX_GCC:-$(dirname "$(command -v "$c")")/$(basename "$c" | sed 's/g++/gcc/')}" ;;
  esac
}
echo "work directory: $work (commands and output: $log)"
case " $parts " in *" 3 "*)
  missing=
  for tool in cmake ninja python3 patch git ar nm; do has $tool || missing="$missing $tool"; done
  [ -n "$compilers" ] || missing="$missing clang++-23/g++-16"
  if [ -n "$missing" ]; then
    bad "part 3 skipped: needs$missing (tools/toolchain/activate.sh; Homebrew: brew install cmake ninja)"
    parts=$(echo " $parts " | sed 's/ 3 / /')
  fi ;;
esac
echo "macOS $(sw_vers -productVersion 2>/dev/null) $(uname -m); Apple clang: $($apple --version 2>/dev/null | head -1)"
for cc in $compilers; do echo "$cc: $($(cxx_of $cc) --version | head -1)"; done
case " $parts " in *" 1 "*)
  echo "== 1. minimal plain-std probes"
  list=
  for cc in $compilers; do list="$list $(cxx_of $cc)"; done
  # PASS: the plain-std control builds and runs; the line also shows what the std::__y1 form does
  # (Linux: align_val_t fails on both, destroying_delete_t, initializer_list, byte and meta on GCC,
  # terminate on Clang; DECISIONS §20.5). A different std::__y1 result is reported as INFO.
  PROBE_OUT=$work/minimal sh "$here/minimal/run-minimal.sh" $list "$apple" >"$work/minimal.txt" 2>&1
  while IFS= read -r line; do
    case $line in
      *"not found"*) info "$line" ;;
      *PLAIN=ok*) ok "$line" ;;
      *) bad "$line" ;;
    esac
  done <"$work/minimal.txt"
  ;;
esac

case " $parts " in *" 2 "*)
  echo "== 2. dyld weak-definition coalescing against Apple's libc++abi"
  d=$work/coalesce
  mkdir -p "$d"
  for cxx in "$apple" $(for cc in $compilers; do [ $cc = clang ] && cxx_of clang; done); do
    tag=clang23
    [ "$cxx" = "$apple" ] && tag=apple-clang
    for v in 1 2 3; do
      exe=$d/coalesce-$tag-$v
      if ! run $cxx -std=c++2c -nostdinc++ -nostdlib++ -DVARIANT=$v "$here/darwin/coalesce.cpp" -o "$exe" -lc++; then
        bad "[$tag] coalesce variant $v: build failed"
        continue
      fi
      out=$("$exe" 2>&1 | tr '\n' ' ')
      echo "$out" >>"$log"
      case $v in
        1) case $out in
             *"current_exception: other"*"type_info: other"*) ok "[$tag] control: plain std, default visibility, is coalesced with libc++abi's (the hazard reproduced): $out" ;;
             *) info "[$tag] control: plain std, default visibility, was NOT coalesced (the hazard did not reproduce): $out" ;;
           esac ;;
        2) case $out in
             *"current_exception: own"*"type_info: own"*) ok "[$tag] std::__y1, default visibility: not coalesced: $out" ;;
             *) bad "[$tag] std::__y1, default visibility: coalesced: $out" ;;
           esac ;;
        3) case $out in
             *"current_exception: own"*"type_info: own"*) ok "[$tag] plain std, hidden: not coalesced: $out" ;;
             *) bad "[$tag] plain std, hidden: coalesced: $out" ;;
           esac ;;
      esac
    done
    # Two images exporting the same std::__y1 inline variable: one object per process?
    for v in 1 2; do
      for n in a b; do
        run $cxx -std=c++2c -nostdinc++ -nostdlib++ -dynamiclib -DVARIANT=$v -DNAME=$n "$here/darwin/weakshare.cpp" \
          -o "$d/lib$n-$tag-$v.dylib" -install_name "$d/lib$n-$tag-$v.dylib"
      done
      run ${APPLE_CC:-/usr/bin/clang} "$here/darwin/weakshare_main.c" -o "$d/weakshare"
      out=$("$d/weakshare" "$d/liba-$tag-$v.dylib" "$d/libb-$tag-$v.dylib" 2>&1)
      if [ $v = 1 ]; then
        info "[$tag] std::__y1 inline variable, default visibility, two dylibs (RTLD_LOCAL): $out (ELF: two objects)"
      else
        [ "$out" = "two objects" ] && ok "[$tag] hidden inline variable, two dylibs: $out" || bad "[$tag] hidden inline variable, two dylibs: $out"
      fi
    done
  done
  # The weak definitions an image exports, as dyld sees them: what a libycxx program exports today.
  ;;
esac

case " $parts " in *" 3 "*)
  echo "== 3. the whole library (Mach-O)"
  for m in static shared; do
    t=$work/y1-$m
    if [ ! -f "$t/.transformed" ]; then
      rm -rf "$t"; mkdir -p "$t"
      git -C "$repo" archive HEAD include src modules cmake tools CMakeLists.txt | tar -x -C "$t" || exit 1
      run patch -p1 -d "$t" -i "$here/plain-std.patch" || { bad "plain-std.patch does not apply"; exit 1; }
      run python3 "$here/transform.py" "$t" $m || exit 1
      touch "$t/.transformed"
    fi
    for cc in $compilers; do
      b=$t/build/$cc
      if [ ! -f "$b/libycxx.a" ]; then
        run cmake -S "$t" -B "$b" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="$(command -v "$(cxx_of $cc)")" \
          -DCMAKE_C_COMPILER="$(cc_of $cc)" -DYCXX_MODULES=OFF -DYCXX_WERROR=OFF -DYCXX_INSTALL=OFF &&
          run ninja -C "$b" -j"$jobs"
      fi
      [ -f "$b/libycxx.a" ] && ok "[$cc] build $m tree" || bad "[$cc] build $m tree"
    done
  done
  for cc in $compilers; do
    cxx=$(cxx_of $cc) c=$(cc_of $cc)
    sh=$work/y1-shared/build/$cc st=$work/y1-static/build/$cc so=$work/so-$cc
    rm -rf "$so"; mkdir -p "$so/ns"
    libgcc= unexport=
    # GCC: -static-libgcc puts libgcc's emulated-TLS runtime in the dylib, whose two entry points
    # are weak exports (dyld could coalesce them with another GCC image's, mixing one image's
    # state with another's entry points). Each image keeps its own: they are not exported
    # (DECISIONS §20, step 4: the exported-symbols list).
    [ $cc = gcc ] && libgcc=-static-libgcc &&
      unexport="-Wl,-unexported_symbol,___emutls_get_address -Wl,-unexported_symbol,___emutls_register_common"
    python3 "$here/nonshared/gen_forward.py" "$so/ns"
    run $c -O2 -fPIC -fexceptions -c "$so/ns/export.c" -o "$so/export.o"
    # tools/ycxx-cxx --libdir: Mach-O has no linker scripts; ld64 reads a file by its contents, so
    # libycxx.a may be the dylib itself and libycxx-abi.a the per-image archive. Set up first: the
    # later checks report their own failures if the dylib or the archive is missing.
    ln -sf "$so/libycxx.0.1.dylib" "$so/libycxx.a"
    ln -sf "$so/libycxx_nonshared.a" "$so/libycxx-abi.a"
    ln -sf "$sh/generated" "$so/generated"
    cp "$sh/ycxx-link-options" "$so/ycxx-link-options"
    # The shared library: install name and versions as the design proposes (absolute here, so that
    # the probes need no rpath; @rpath is checked below). The ABI runtime's members are linked as
    # objects, without rtti_float16.cpp.o: the _Float16 type_info objects are per image
    # (libycxx_nonshared.a, DECISIONS §20.6), and GCC on Darwin also emits them, non-weak, with
    # rtti.cpp.o (a duplicate a whole-archive link exposes; fixed on the shared-lib branch).
    mkdir -p "$so/abi"
    (cd "$so/abi" && ar x "$sh/libycxx-abi.a" && rm -f rtti_float16.cpp.o)
    linked=0
    if run $cxx -dynamiclib -o "$so/libycxx.0.1.dylib" -install_name "$so/libycxx.0.1.dylib" \
         -compatibility_version 0.1 -current_version 0.1.0 "$so/export.o" \
         -Wl,-force_load,"$sh/libycxx.a" "$so"/abi/*.o -nostdlib++ $libgcc $unexport; then
      ok "[$cc] link libycxx.0.1.dylib"; linked=1
    else
      bad "[$cc] link libycxx.0.1.dylib (the checks that need it fail too)"
    fi
    (cd "$so/ns" && ar x "$sh/libycxx.a" $(ar t "$sh/libycxx.a" | grep -E '^(new|delete)[a-z_]*\.cpp\.o$|^allocation_table\.cpp\.o$')) &&
    (cd "$so/ns" && ar x "$sh/libycxx-abi.a" rtti_float16.cpp.o)
    run $c -O2 -fPIC -fexceptions -c "$so/ns/forward.c" -o "$so/ns/forward.o" &&
      run "$work/y1-shared/tools/ycxx-cxx" $cc --libdir="$sh" -O2 -fPIC -frtti -I"$work/y1-shared/src/abi" \
        -I"$sh/generated" -c "$here/nonshared/rtti.cpp" -o "$so/ns/rtti.o" &&
      run ar rcs "$so/libycxx_nonshared.a" "$so"/ns/*.o && ok "[$cc] build libycxx_nonshared.a" || bad "[$cc] build libycxx_nonshared.a"
    [ $linked = 1 ] || continue
    n=$(nm -gU "$so/libycxx.0.1.dylib" | wc -l | tr -d ' ')
    foreign=$(nm -gU "$so/libycxx.0.1.dylib" | awk '{print $NF}' | grep -vE 'St4__y1|6__ycxx|^___ycxx_allocation_functions$|^___ycxx_abi_' | tr '\n' ' ')
    [ -z "$foreign" ] && ok "[$cc] libycxx.0.1.dylib exports $n symbols, all std::__y1, __ycxx, __ycxx_abi_* or the allocation table" ||
      bad "[$cc] libycxx.0.1.dylib also exports: $foreign"
    weak=$(nm -m "$so/libycxx.0.1.dylib" | grep -c 'weak external' | tr -d ' ')
    weakforeign=$(nm -m "$so/libycxx.0.1.dylib" | grep 'weak external' | awk '{print $NF}' | grep -vE 'St4__y1|6__ycxx|^___ycxx_allocation_functions$' | tr '\n' ' ')
    [ -z "$weakforeign" ] && ok "[$cc] libycxx.0.1.dylib's $weak weak exports are all std::__y1, __ycxx or the allocation table (weak by design, DECISIONS §2)" ||
      bad "[$cc] libycxx.0.1.dylib exports weak definitions outside std::__y1: $weakforeign"
    # @rpath and the compatibility version: a program linked against an @rpath install name.
    cp "$so/libycxx.0.1.dylib" "$so/rp.dylib" && run install_name_tool -id @rpath/libycxx.0.1.dylib "$so/rp.dylib"
    mkdir -p "$so/rpath" && cp "$so/rp.dylib" "$so/rpath/libycxx.0.1.dylib"
    printf 'int main() { return 0; }\n' >"$so/rp.c"
    if run $c "$so/rp.c" -o "$so/rp" "$so/rpath/libycxx.0.1.dylib" -Wl,-rpath,"$so/rpath" && "$so/rp"; then
      ok "[$cc] a program linked against @rpath/libycxx.0.1.dylib loads it ($(otool -L "$so/rp" | grep libycxx | sed 's/^[[:space:]]*//'))"
    else
      bad "[$cc] @rpath/libycxx.0.1.dylib"
    fi
  done
  export DYLD_LIBRARY_PATH="$work/so-clang:$work/so-gcc${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
  # The compiler-known entities, both modes.
  for cc in $compilers; do
    for m in static shared; do
      if [ $m = static ]; then libdir=$work/y1-static/build/$cc; else libdir=$work/so-$cc; fi
      YCXX_LIBDIR=$libdir PROBE_OUT=$work/known-$m PROBE_MODULES=${PROBE_MODULES:-} sh "$here/run-known.sh" "$work/y1-$m" $cc \
        >"$work/known-$m-$cc.txt" 2>&1
      while IFS= read -r line; do
        case $line in
          *dangling_warning:\ COMPILE\ FAILED*dangling*) ok "$m $line" ;;
          *": n/a"*|*"skipped"*) ;;
          *"run: ok"*) ok "$m $line" ;;
          *) bad "$m $line" ;;
        esac
      done <"$work/known-$m-$cc.txt"
    done
  done
  # Coexistence with Apple's libc++ (and Homebrew's libstdc++), and plugins across modes.
  for cc in $compilers; do
    d=$work/coexist-$cc
    mkdir -p "$d"
    ycxx="$work/y1-shared/tools/ycxx-cxx $cc --libdir=$work/so-$cc"
    run $ycxx -O2 -fPIC -dynamiclib "$here/coexist/mine.cpp" -o "$d/libmine.dylib" -install_name "$d/libmine.dylib" ||
      bad "[$cc] build libmine.dylib"
    others=libcxx
    [ $cc = gcc ] && others="libcxx libstdcxx"
    for o in $others; do
      if [ $o = libcxx ]; then ocxx="$apple -stdlib=libc++"; else ocxx=$(cxx_of gcc); fi
      run $ocxx -std=c++20 -O2 -dynamiclib "$here/coexist/other.cpp" -o "$d/libother-$o.dylib" -install_name "$d/libother-$o.dylib" ||
        bad "[$cc] build libother-$o.dylib"
      run $ycxx -O2 "$here/coexist/main.cpp" "$here/coexist/mine.cpp" -o "$d/prog-$o" "$d/libother-$o.dylib"
      out=$("$d/prog-$o" 2>&1); [ "$out" = "mine 31 other 31" ] && ok "[$cc/$o] libycxx program + $o library: $out" ||
        bad "[$cc/$o] libycxx program + $o library: $out"
      run $ocxx -std=c++20 -O2 "$here/coexist/main.cpp" -o "$d/host-$o" "$d/libother-$o.dylib" "$d/libmine.dylib"
      out=$("$d/host-$o" 2>&1); [ "$out" = "mine 31 other 31" ] && ok "[$cc/$o] $o program + both libraries: $out" ||
        bad "[$cc/$o] $o program + both libraries: $out"
      run $ocxx -std=c++20 -O2 "$here/coexist/main.cpp" -o "$d/host2-$o" "$d/libmine.dylib" "$d/libother-$o.dylib"
      out=$("$d/host2-$o" 2>&1); [ "$out" = "mine 31 other 31" ] && ok "[$cc/$o] $o program + both libraries (libycxx first): $out" ||
        bad "[$cc/$o] $o program + both libraries (libycxx first): $out"
      run ${APPLE_CC:-/usr/bin/clang} -O2 "$here/coexist/dl.c" -o "$d/dl"
      # The control: the other library alone in the C host, no libycxx in the process. A
      # libstdc++ dylib scores 27 there: its operator new/delete imports are weak-def-coalesce,
      # so dyld binds them to Apple's libc++abi (which libSystem loads), whose new_handler runs and
      # whose bad_alloc libstdc++'s handler does not catch (§2's Darwin hazard, on libstdc++
      # itself). libycxx's presence must then leave the other side's score as the control has it.
      control=$(DL_MODE=global "$d/dl" "$d/libother-$o.dylib" 2>&1 | sed -n 's/.*other \([-0-9]*\)$/\1/p')
      if [ "$control" != 31 ]; then
        info "[$cc/$o] control: libother-$o.dylib alone in a C host scores $control, without libycxx (for libstdc++: its operator new binds to Apple's libc++abi by weak-definition coalescing)"
      fi
      for mode in global local; do
        for order in "libmine.dylib libother-$o.dylib" "libother-$o.dylib libmine.dylib"; do
          set -- $order
          out=$(DL_MODE=$mode "$d/dl" "$d/$1" "$d/$2" 2>&1)
          if [ "$out" = "mine 31 other 31" ]; then
            ok "[$cc/$o] dlopen $1 then $2 ($mode): $out"
          elif [ "$out" = "mine 31 other $control" ]; then
            ok "[$cc/$o] dlopen $1 then $2 ($mode): $out (the other side scores as in its control, without libycxx)"
          else
            bad "[$cc/$o] dlopen $1 then $2 ($mode): $out (control: other $control)"
          fi
        done
      done
    done
    p=$work/plugin-$cc
    mkdir -p "$p"
    for m in static shared; do
      if [ $m = static ]; then libdir=$work/y1-static/build/$cc; else libdir=$work/so-$cc; fi
      y="$work/y1-$m/tools/ycxx-cxx $cc --libdir=$libdir"
      attr=
      [ $m = static ] && attr=-DPLUGIN_EXPORT_ATTRIBUTE
      run $y -O2 -fPIC -dynamiclib $attr "$here/plugin/plugin.cpp" -o "$p/plugin-$m.dylib" -install_name "$p/plugin-$m.dylib" ||
        bad "[$cc] build the $m plugin"
      run $y -O2 $attr "$here/plugin/host.cpp" -o "$p/host-$m" || bad "[$cc] build the $m host"
      echo "INFO [$cc] $m plugin exports $(nm -gU "$p/plugin-$m.dylib" | wc -l | tr -d ' ') symbols, $(nm -gU "$p/plugin-$m.dylib" | grep -c 'St4__y1') of them std::__y1, $(nm -m "$p/plugin-$m.dylib" | grep -c 'weak external') weak"
    done
    for hm in static shared; do
      for pm in static shared; do
        out=$("$p/host-$hm" "$p/plugin-$pm.dylib" 2>&1)
        want="plugin 15 shared-state 0"
        [ $hm$pm = sharedshared ] && want="plugin 15 shared-state 15"
        [ "$out" = "$want" ] && ok "[$cc] $hm host + $pm plugin: $out" || bad "[$cc] $hm host + $pm plugin: $out (expected $want)"
      done
    done
    echo "INFO [$cc] a static-mode libycxx program exports $(nm -gU "$p/host-static" | wc -l | tr -d ' ') symbols ($(nm -m "$p/host-static" | grep -c 'weak external') weak); a shared-mode one $(nm -gU "$p/host-shared" | wc -l | tr -d ' ') ($(nm -m "$p/host-shared" | grep -c 'weak external') weak, $(nm -gU "$p/host-shared" | grep -c 'St4__y1') std::__y1)"
  done
  ;;
esac

echo "passed $pass, failed $fail (log: $log)"
[ $fail = 0 ]
