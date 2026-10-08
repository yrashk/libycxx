#!/bin/sh
# check-headers.sh TREE [gcc|clang]: compiles each public header of a transformed tree alone
# (-fsyntax-only) and prints the first error of each header that fails.
tree=$1; shift
repo=$(cd "$(dirname "$0")/../../.." && pwd)
for cc in ${*:-gcc clang}; do
  case $cc in
    gcc) cxx=${YCXX_GXX:-g++-16}; extra=-Wno-attributes ;;
    clang) cxx=${YCXX_CLANGXX:-clang++-23}; extra= ;;
  esac
  gen=$tree/build/$cc/generated/include
  [ -d "$gen" ] || gen=$repo/build/$cc/generated/include
  fails=0 total=0
  for h in "$tree"/include/*; do
    [ -f "$h" ] || continue
    name=$(basename "$h")
    case $name in *.h) continue ;; esac
    total=$((total + 1))
    out=$(echo "#include <$name>" | $cxx $extra -std=c++26 -nostdinc++ -isystem "$tree/include" -isystem "$gen" \
          -fsyntax-only -x c++ - 2>&1)
    if [ $? -ne 0 ]; then
      fails=$((fails + 1))
      echo "[$cc] <$name>: $(echo "$out" | grep -m1 'error')"
    fi
  done
  echo "[$cc] $fails of $total headers fail"
done
