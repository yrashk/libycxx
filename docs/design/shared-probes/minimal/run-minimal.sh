#!/bin/sh
# run-minimal.sh [compiler...]: the standalone plain-std probes (README), for each compiler given
# (default: g++-16 clang++-23). Prints, per probe and compiler, whether the std::__y1 form works
# ("y1 ok") or not ("y1 FAILS: <stage>"), after checking that the plain form works.
here=$(cd "$(dirname "$0")" && pwd)
out=${PROBE_OUT:-$(mktemp -d "${TMPDIR:-/tmp}/ycxx-minimal.XXXXXX")}
mkdir -p "$out"
for cxx in ${*:-g++-16 clang++-23}; do
  command -v ${cxx%% *} >/dev/null 2>&1 || { echo "[$cxx] not found"; continue; }
  family=clang
  $cxx --version 2>/dev/null | grep -qi 'free software foundation\|gcc' && family=gcc
  for src in "$here"/*.cpp; do
    name=$(basename "$src" .cpp)
    only=$(sed -n 's|^// COMPILERS: *\([a-z]*\).*|\1|p' "$src")
    [ -n "$only" ] && [ "$only" != "$family" ] && continue
    flags=$(sed -n 's|^// FLAGS: *||p' "$src")
    res=
    for form in PLAIN Y1; do
      exe=$out/$family-$name-$form
      if ! $cxx -std=c++26 $flags -D$form -nostdinc++ -nostdlib++ "$src" -o "$exe" >"$exe.log" 2>&1; then
        if grep -q 'undefined\|Undefined' "$exe.log"; then
          r="link: $(grep -m1 -E "undefined reference to|^ +\"_" "$exe.log" | sed 's|.*undefined reference to ||; s|^ *||' | cut -c1-60)"
        else r="compile: $(grep -m1 'error' "$exe.log" | sed 's|.*error: ||' | cut -c1-90)"; fi
      elif ! "$exe" >>"$exe.log" 2>&1; then r="run: wrong result"
      else r=ok; fi
      res="$res $form=$r"
    done
    echo "[$cxx] $name:$res"
  done
done
