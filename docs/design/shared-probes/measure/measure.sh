#!/bin/sh
# measure.sh TREE... : compile time (best of 3, -O2 -c), object size, binary size, symbol-name
# bytes (.strtab of the linked program) and debug-info size (-g) of sample.cpp against each tree
# (the repository, or a tree transform.py rewrote), for each compiler: the cost of the inline
# namespace's longer mangled names. Static mode (each tree's own build/<compiler> archives).
here=$(cd "$(dirname "$0")" && pwd)
out=${PROBE_OUT:-$(mktemp -d "${TMPDIR:-/tmp}/ycxx-measure.XXXXXX")}
mkdir -p "$out"
for tree in "$@"; do
  tree=$(cd "$tree" && pwd)
  for cc in gcc clang; do
    best=
    for i in 1 2 3; do
      s=$(date +%s.%N)
      "$tree/tools/ycxx-cxx" $cc -O2 -c "$here/sample.cpp" -o "$out/s.o" || exit 1
      e=$(date +%s.%N)
      t=$(echo "$e - $s" | bc)
      [ -z "$best" ] || [ "$(echo "$t < $best" | bc)" = 1 ] && best=$t
    done
    "$tree/tools/ycxx-cxx" $cc -O2 "$out/s.o" -o "$out/s" || exit 1
    "$tree/tools/ycxx-cxx" $cc -O2 -g -c "$here/sample.cpp" -o "$out/sg.o" || exit 1
    strtab=$(readelf -SW "$out/s" | awk '$2==".strtab"{print strtonum("0x"$6)}')
    obj=$(stat -c %s "$out/s.o")
    bin=$(stat -c %s "$out/s")
    strip -o "$out/s.stripped" "$out/s"; stripped=$(stat -c %s "$out/s.stripped")
    dbg=$(readelf -SW "$out/sg.o" | awk '$2 ~ /^\.debug_(info|str|line_str)$/ {s += strtonum("0x"$6)} END {print s}')
    printf '%s %s: compile %.2fs, object %d, program %d (stripped %d), .strtab %d, debug info+strings %d\n' \
      "$(basename "$tree")" $cc "$best" $obj $bin $stripped $strtab $dbg
  done
done
