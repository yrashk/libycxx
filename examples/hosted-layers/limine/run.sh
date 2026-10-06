#!/bin/sh
# Example B end to end: builds the kernel (CMake, toolchain.cmake), makes a bootable ISO with
# Limine, boots it in QEMU with COM1 on standard output, and checks what the kernel printed.
#
#   examples/hosted-layers/limine/run.sh [gcc|clang]       (default gcc)
#
# Needs cmake, ninja, git, a C compiler (Limine's `limine` tool is built from its release),
# xorriso and qemu-system-x86_64. Limine's binary release (the v11.4.1-binary tag of
# github.com/limine-bootloader/limine, BSD-licensed) is fetched once into $LIMINE_DIR (default
# build/limine-v11.4.1-binary of the checkout); set LIMINE_DIR to a copy to work offline.
# Environment: QEMU_TIMEOUT (seconds, default 120), QEMU (default qemu-system-x86_64),
# BUILD_DIR (default build/hosted-layers-limine-<compiler>).
#
# Passes (exit 0) when QEMU ends through the isa-debug-exit device with the kernel's success code
# (QEMU's status 33 = (0x10 << 1) | 1) and the serial output has "hosted-layers demo: ok".
set -eu
here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../../.." && pwd)
compiler=${1:-gcc}
limine_version=v11.4.1
limine_dir=${LIMINE_DIR:-$repo/build/limine-$limine_version-binary}
build=${BUILD_DIR:-$repo/build/hosted-layers-limine-$compiler}
qemu=${QEMU:-qemu-system-x86_64}
timeout_s=${QEMU_TIMEOUT:-120}

say() { printf '== %s\n' "$*"; }
run() { printf '$ %s\n' "$*"; "$@"; }

for tool in cmake ninja xorriso "$qemu"; do
  command -v "$tool" >/dev/null 2>&1 || { echo "run.sh: $tool is not installed" >&2; exit 2; }
done

# 1. Limine: the binary release, and its host tool (bios-install).
if [ ! -f "$limine_dir/limine-bios-cd.bin" ]; then
  say "fetching Limine $limine_version (binary release)"
  run git clone --quiet --depth 1 --branch "$limine_version-binary" \
    https://github.com/limine-bootloader/limine "$limine_dir"
fi
if [ ! -x "$limine_dir/limine" ]; then
  say "building Limine's host tool"
  run make -C "$limine_dir" CC="${CC:-cc}" >/dev/null
fi

# 2. The kernel.
say "building the kernel with $compiler"
run cmake -S "$here" -B "$build" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$here/toolchain.cmake" \
  -DYCXX_COMPILER="$compiler" -DCMAKE_BUILD_TYPE=Release >"$build.configure.log" 2>&1 ||
  { cat "$build.configure.log"; exit 1; }
run cmake --build "$build" -j "${JOBS:-4}"

# 3. A hybrid BIOS/UEFI ISO (Limine's USAGE.md).
say "making the ISO"
iso_root=$build/iso_root
rm -rf "$iso_root"
mkdir -p "$iso_root/boot/limine" "$iso_root/EFI/BOOT"
cp "$build/kernel.elf" "$iso_root/boot/kernel.elf"
cp "$here/limine.conf" "$limine_dir/limine-bios.sys" "$limine_dir/limine-bios-cd.bin" \
  "$limine_dir/limine-uefi-cd.bin" "$iso_root/boot/limine/"
cp "$limine_dir/BOOTX64.EFI" "$iso_root/EFI/BOOT/"
iso=$build/hosted-layers.iso
run xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 \
  -boot-info-table -hfsplus -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
  -efi-boot-part --efi-boot-image --protective-msdos-label "$iso_root" -o "$iso" 2>"$build/xorriso.log" ||
  { cat "$build/xorriso.log"; exit 1; }
run "$limine_dir/limine" bios-install "$iso" >"$build/bios-install.log" 2>&1 ||
  { cat "$build/bios-install.log"; exit 1; }

# 4. Boot it: no display, COM1 on standard output (copied to serial.log), the exit device.
say "booting in QEMU (timeout ${timeout_s}s); serial output follows"
log=$build/serial.log
status_file=$build/qemu.status
rm -f "$log" "$status_file"
{
  st=0
  timeout "$timeout_s" "$qemu" -M q35 -m 256M -cdrom "$iso" -boot d -display none -no-reboot \
    -serial stdio -monitor none -device isa-debug-exit,iobase=0xf4,iosize=0x04 </dev/null || st=$?
  echo "$st" >"$status_file"
} | tee "$log"
st=$(cat "$status_file")
say "QEMU exited with status $st"
if [ "$st" = 33 ] && grep -q 'hosted-layers demo: ok' "$log"; then
  say "PASS: the kernel ran the demonstration and reported success"
  exit 0
fi
case $st in
  124) say "FAIL: QEMU timed out" ;;
  35) say "FAIL: the kernel reported a failure (isa-debug-exit 0x11)" ;;
  *) say "FAIL: unexpected QEMU status $st, or no success line in $log" ;;
esac
exit 1
