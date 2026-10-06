#!/bin/sh
# Freestanding check: every core header (and every header [compliance] requires) must compile in a TU built with
#   -ffreestanding -nostdlib -nostdinc -fno-exceptions -fno-rtti
# plus the compiler's own header directory (core uses its <stddef.h>, DECISIONS §3; no C library)
# and the smoke test must link with no C library for bare-metal targets.
set -e
repo=$(cd "$(dirname "$0")/.." && pwd)
. "$repo/tools/lib/ui.sh"
. "$repo/tools/lib/env.sh"
ycxx_env_load
# Compilers: the YCXX_* variables of tools/toolchain/activate.*, else the -16/-23 names.
gcc=${YCXX_GCC:-gcc-16} gxx=${YCXX_GXX:-g++-16}
clang=${YCXX_CLANG:-clang-23} clangxx=${YCXX_CLANGXX:-clang++-23}
lld=${YCXX_LLD:-ld.lld-23} llvm_ar=${YCXX_LLVM_AR:-llvm-ar-23}
out=$repo/build/freestanding
mkdir -p "$out"
# Every core header, every header with a freestanding subset, and every header of [compliance]'s
# Table 27 (some of which, such as <exception> and <typeinfo>, need the ABI runtime only when used
# with exceptions or RTTI).
cores=$(python3 -c "import sys; sys.path.insert(0,'$repo/tools')
from headers import CORE, FREESTANDING_SUBSET, FREESTANDING_REQUIRED
print(' '.join(dict.fromkeys(CORE + FREESTANDING_SUBSET + FREESTANDING_REQUIRED)))")
base_flags="-std=c++26 -ffreestanding -nostdinc -nostdinc++ -isystem $repo/include -fno-exceptions -fno-rtti -O2 -Wall -Wextra -Werror"
fail=0
run() { # compiler-command target-label C-compiler linker [skip-link-reason]
  cc=$1; label=$2   # $3: C compiler for rt.c, $4: linker; $5: when set, no smoke link (why)
  # -nostdinc drops the compiler's own headers too; core needs its <stddef.h> (::max_align_t).
  flags="$base_flags -isystem $($cc -print-file-name=include)"
  ui_section "Freestanding: ${label}"
  ui_cmd $cc $flags -c "<each header>"
  nbad=0
  for h in $cores; do
    printf '#include <%s>\nint ycxx_header_check_%s;\n' "$h" "$(echo $h | tr -c 'a-z0-9\n' '_')" > "$out/h_$h.cpp"
    if ! $cc $flags -c "$out/h_$h.cpp" -o "$out/h_$h.$label.o" 2> "$out/h_$h.$label.log"; then
      ui_fail "[${label}] <$h>"; sed 's/^/    /' "$out/h_$h.$label.log" | head -10; fail=1; nbad=$((nbad + 1))
    fi
  done
  [ $nbad = 0 ] && ui_ok "[${label}] all $(echo $cores | wc -w | tr -d ' ') core and freestanding headers compile"
  if [ -n "${5:-}" ]; then
    ui_skip "[${label}] smoke link" "($5)"
    return
  fi
  ui_cmd $4 smoke.o smoke_o0.o rt.o fsrt.a -o "$out/smoke.$label.elf"
  if $cc $flags -c "$repo/tests/freestanding/smoke.cpp" -o "$out/smoke.$label.o" 2> "$out/smoke.$label.log" &&
     $cc $flags -O0 -c "$repo/tests/freestanding/smoke_o0.cpp" -o "$out/smoke_o0.$label.o" 2>> "$out/smoke.$label.log" &&
     $3 -ffreestanding -nostdlib -O2 -c "$repo/tests/freestanding/rt.c" -o "$out/rt.$label.o" &&
     build_fsrt "$cc" "$label" 2>> "$out/smoke.$label.log" &&
     $4 "$out/smoke.$label.o" "$out/smoke_o0.$label.o" "$out/rt.$label.o" "$out/fsrt.$label.a" \
        -o "$out/smoke.$label.elf" 2>> "$out/smoke.$label.log"; then
    ui_ok "[${label}] smoke program and freestanding runtime link with no C library"
  else
    ui_fail "[${label}] smoke"; sed 's/^/    /' "$out/smoke.$label.log" | head -20; fail=1
  fi
}
# libycxx-freestanding.a for one target: the allocation-function defaults, std::nothrow and
# floating-point <charconv>, the <atomic> lock and wait tables, <debugging> and the default PAL
# wait and debugger hooks. Hidden visibility, as the hosted archives (DECISIONS §2).
build_fsrt() {
  rm -rf "$out/fsrt.$2" && mkdir -p "$out/fsrt.$2"
  for f in "$repo"/src/runtime/new/*.cpp "$repo"/src/freestanding/new/*.cpp "$repo"/src/runtime/charconv/*.cpp \
           "$repo"/src/runtime/atomic/*.cpp "$repo"/src/runtime/debugging/*.cpp \
           "$repo"/src/runtime/contracts/*.cpp "$repo"/src/freestanding/contracts/*.cpp "$repo"/src/freestanding/pal/*.cpp; do
    # One function per file; GCC: no zero fill of the charconv work buffers (CMakeLists.txt).
    case "$2" in gcc*) nw="-Wno-sized-deallocation -ftrivial-auto-var-init=uninitialized" ;; *) nw= ;; esac
    $1 $flags $nw -c "$f" -o "$out/fsrt.$2/$(basename "$f" .cpp).o" || return 1
  done
  rm -f "$out/fsrt.$2.a" && $llvm_ar rcs "$out/fsrt.$2.a" "$out/fsrt.$2"/*.o
}
run "$clangxx --target=x86_64-unknown-none-elf" clang-x86_64 "$clang --target=x86_64-unknown-none-elf" "$lld -e _start"
run "$clangxx --target=riscv64-unknown-elf -march=rv64gc -mabi=lp64d" clang-riscv64 "$clang --target=riscv64-unknown-elf -march=rv64gc -mabi=lp64d" "$lld -e _start"
# GCC lowers some builtins (e.g. __builtin_popcountll without -mpopcnt) to libgcc helpers, so
# freestanding programs built with GCC link libgcc, as GCC itself requires.
# The host GCC's own target: the bare-metal link needs ELF objects, so a GCC producing Mach-O or
# PE (Homebrew's on macOS) checks the headers only; an ELF cross GCC can be named by YCXX_GXX.
gcc_target=$($gxx -dumpmachine)
case $gcc_target in
  *darwin*|*mingw*|*cygwin*|*windows*) gcc_nolink="GCC targets $gcc_target, not ELF: a bare-metal link needs an ELF cross GCC" ;;
  *) gcc_nolink= ;;
esac
run "$gxx" "gcc-${gcc_target%%-*}" "$gcc" "$lld -e _start $($gcc -print-libgcc-file-name)" "$gcc_nolink"
exit $fail
