#!/bin/sh
# Freestanding check: every core header must compile in a TU built with
#   -ffreestanding -nostdlib -nostdinc -fno-exceptions -fno-rtti
# and the smoke test must link with no C library for bare-metal targets.
set -e
# Compilers: the YCXX_* variables of tools/toolchain/activate.*, else the -16/-23 names.
gcc=${YCXX_GCC:-gcc-16} gxx=${YCXX_GXX:-g++-16}
clang=${YCXX_CLANG:-clang-23} clangxx=${YCXX_CLANGXX:-clang++-23}
lld=${YCXX_LLD:-ld.lld-23} llvm_ar=${YCXX_LLVM_AR:-llvm-ar-23}
repo=$(cd "$(dirname "$0")/.." && pwd)
out=$repo/build/freestanding
mkdir -p "$out"
cores=$(python3 -c "import sys; sys.path.insert(0,'$repo/tools'); from headers import CORE; print(' '.join(CORE))")
flags="-std=c++26 -ffreestanding -nostdinc -nostdinc++ -isystem $repo/include -fno-exceptions -fno-rtti -O2 -Wall -Wextra -Werror"
fail=0
run() { # compiler-command target-label
  cc=$1; label=$2   # $3: C compiler for rt.c, $4: linker
  for h in $cores; do
    printf '#include <%s>\nint ycxx_header_check_%s;\n' "$h" "$(echo $h | tr -c 'a-z0-9\n' '_')" > "$out/h_$h.cpp"
    if ! $cc $flags -c "$out/h_$h.cpp" -o "$out/h_$h.$label.o" 2> "$out/h_$h.$label.log"; then
      echo "FAIL [$label] <$h>"; sed 's/^/    /' "$out/h_$h.$label.log" | head -10; fail=1
    fi
  done
  if $cc $flags -c "$repo/tests/freestanding/smoke.cpp" -o "$out/smoke.$label.o" 2> "$out/smoke.$label.log" &&
     $cc $flags -O0 -c "$repo/tests/freestanding/smoke_o0.cpp" -o "$out/smoke_o0.$label.o" 2>> "$out/smoke.$label.log" &&
     $3 -ffreestanding -nostdlib -O2 -c "$repo/tests/freestanding/rt.c" -o "$out/rt.$label.o" &&
     build_fsrt "$cc" "$label" 2>> "$out/smoke.$label.log" &&
     $4 "$out/smoke.$label.o" "$out/smoke_o0.$label.o" "$out/rt.$label.o" "$out/fsrt.$label.a" \
        -o "$out/smoke.$label.elf" 2>> "$out/smoke.$label.log"; then
    echo "ok   [$label] all core headers + smoke link"
  else
    echo "FAIL [$label] smoke"; sed 's/^/    /' "$out/smoke.$label.log" | head -20; fail=1
  fi
}
# libycxx-freestanding.a for one target: the allocation-function defaults, std::nothrow and
# floating-point <charconv>, the <atomic> lock and wait tables, <debugging> and the default PAL
# wait and debugger hooks.
build_fsrt() {
  rm -rf "$out/fsrt.$2" && mkdir -p "$out/fsrt.$2"
  for f in "$repo"/src/runtime/new/*.cpp "$repo"/src/freestanding/new/*.cpp "$repo"/src/runtime/charconv/*.cpp \
           "$repo"/src/runtime/atomic/*.cpp "$repo"/src/runtime/debugging/*.cpp \
           "$repo"/src/runtime/contracts/*.cpp "$repo"/src/freestanding/contracts/*.cpp "$repo"/src/freestanding/pal/*.cpp; do
    case "$2" in gcc*) nw=-Wno-sized-deallocation ;; *) nw= ;; esac # one function per file
    $1 $flags $nw -c "$f" -o "$out/fsrt.$2/$(basename "$f" .cpp).o" || return 1
  done
  rm -f "$out/fsrt.$2.a" && $llvm_ar rcs "$out/fsrt.$2.a" "$out/fsrt.$2"/*.o
}
run "$clangxx --target=x86_64-unknown-none-elf" clang-x86_64 "$clang --target=x86_64-unknown-none-elf" "$lld -e _start"
run "$clangxx --target=riscv64-unknown-elf -march=rv64gc -mabi=lp64d" clang-riscv64 "$clang --target=riscv64-unknown-elf -march=rv64gc -mabi=lp64d" "$lld -e _start"
# GCC lowers some builtins (e.g. __builtin_popcountll without -mpopcnt) to libgcc helpers, so
# freestanding programs built with GCC link libgcc, as GCC itself requires.
run "$gxx" gcc-x86_64 "$gcc" "$lld -e _start $($gcc -print-libgcc-file-name)"
exit $fail
