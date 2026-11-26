#!/bin/sh
# Check the relocated consumers built by tests/cmake/run.sh.
set -eu
if [ "$#" -ne 2 ]; then
  echo "usage: $0 gcc|clang libstdc++|libc++" >&2
  exit 2
fi
compiler=$1
expected=$2
case $expected in
  libstdc++) wanted='libstdc\+\+'; foreign='(libc\+\+(abi)?|libycxx)' ;;
  libc++) wanted='libc\+\+'; foreign='(libstdc\+\+|libycxx)' ;;
  *) echo "unknown C++ runtime '$expected'" >&2; exit 2 ;;
esac

case $(uname -s) in
  Darwin)
    dependencies() { otool -L "$1"; }
    defined_symbols() { nm -U "$1"; }
    suffix='(\.[0-9]+)*\.dylib'
    resolution='[[:space:]]'
    backend=libycxx_execution_portable.dylib
    ;;
  Linux)
    dependencies() { ldd "$1"; }
    defined_symbols() { nm --defined-only "$1"; }
    suffix='\.so(\.[0-9]+)*'
    resolution='[[:space:]]+=>[[:space:]]+/'
    backend=libycxx_execution_portable.so
    ;;
  *) echo "unsupported host for portable runtime verification" >&2; exit 2 ;;
esac

repo=$(cd "$(dirname "$0")/../.." && pwd)
build=${YCXX_CMAKE_TEST_DIR:-$repo/build/cmake-test}
for shared in OFF ON; do
  fixture="$build/$compiler/execution-portable-install-$shared"
  for program in portable_stop_token portable_execution portable_execution_module; do
    executable="$fixture/consumer/$program"
    libs=$(dependencies "$executable")
    symbols=$(defined_symbols "$executable")
    if [ "$shared" = ON ] && [ "$program" != portable_stop_token ]; then
      libs="$libs
$(dependencies "$fixture/prefix/lib/$backend")"
      symbols="$symbols
$(defined_symbols "$fixture/prefix/lib/$backend")"
    fi
    label="$compiler BUILD_SHARED_LIBS=$shared $program"
    if ! printf '%s\n' "$libs" | grep -E "(^|[[:space:]/])${wanted}${suffix}${resolution}" >/dev/null; then
      echo "$label did not link stock $expected" >&2
      exit 1
    fi
    if printf '%s\n' "$libs" | grep -E "(^|[[:space:]/])${foreign}${suffix}[[:space:]]" >/dev/null ||
       printf '%s\n' "$symbols" | grep -E '[[:space:]]__+ycxx_linkage_(static|shared)_v1$' >/dev/null; then
      echo "$label brings in a second C++ runtime" >&2
      exit 1
    fi
    echo "$label uses stock $expected"
  done
done
