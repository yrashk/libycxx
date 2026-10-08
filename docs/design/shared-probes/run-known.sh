#!/bin/sh
# run-known.sh TREE [gcc|clang ...]: the compiler-known entities probe (DECISIONS §20,
# "What must stay plain std"). Compiles, links and runs each known/*.cpp against TREE (the
# repository, or a tree transform.py rewrote; TREE/build/<compiler> holds its libycxx build) and
# lists the plain-std symbols (`std::` but not `std::__y1::`) each object refers to or defines.
# Directives in a probe: `// FLAGS: ...`, `// COMPILERS: gcc`, `// MODULES` (import std).
tree=$(cd "$1" && pwd); shift
here=$(cd "$(dirname "$0")" && pwd)
out=${PROBE_OUT:-$(mktemp -d)}
mkdir -p "$out"
for cc in ${*:-gcc clang}; do
  if [ -n "${PROBE_MODULES:-}" ] && [ ! -f "$out/modules-$cc/flags" ]; then
    "$tree/tools/ycxx-modules" $cc -o "$out/modules-$cc" -O2 >"$out/modules-$cc.log" 2>&1 ||
      echo "[$cc] modules: build failed ($out/modules-$cc.log)"
  fi
  for src in "$here"/known/*.cpp; do
    name=$(basename "$src" .cpp)
    only=$(sed -n 's|^// COMPILERS: *\([a-z]*\).*|\1|p' "$src")
    if [ -n "$only" ] && [ "$only" != "$cc" ]; then
      echo "[$cc] $name: n/a"
      continue
    fi
    flags=$(sed -n 's|^// FLAGS: *||p' "$src")
    mods=
    if grep -q '^// MODULES' "$src"; then
      [ -n "${PROBE_MODULES:-}" ] || { echo "[$cc] $name: skipped (PROBE_MODULES=1)"; continue; }
      mods="--std-modules=$out/modules-$cc"
    fi
    case " $flags " in *" -O0 "*) ;; *) flags="-O2 $flags" ;; esac
    obj=$out/$cc-$name.o
    if ! "$tree/tools/ycxx-cxx" $cc $mods $flags -c "$src" -o "$obj" >"$out/$cc-$name.log" 2>&1; then
      echo "[$cc] $name: COMPILE FAILED: $(grep -m1 'error' "$out/$cc-$name.log" | cut -c1-200)"
      continue
    fi
    # Plain-std symbols: St<len> (not St4__y1) after the _Z prefix or inside a name.
    plain=$(nm "$obj" | awk '{print $NF}' | grep -E 'St[0-9]' | grep -vE '^_ZNSt4__y1|^_ZNKSt4__y1' |
            grep -E '(^_Z(N|NK)?St[0-9]|^_ZT[ISV]St[0-9]|St[0-9]+[a-z_]+)' | grep -vE 'St4__y1' | sort -u | tr '\n' ' ')
    # Out-of-line std::move/std::forward (the builtin_std_functions probe).
    calls=$(nm "$obj" | awk '{print $NF}' | grep -E '4move|7forward|9addressof|8as_const' | tr '\n' ' ')
    if ! "$tree/tools/ycxx-cxx" $cc $mods $flags "$src" -o "$out/$cc-$name" >>"$out/$cc-$name.log" 2>&1; then
      echo "[$cc] $name: LINK FAILED: $(grep -m1 -E 'error|undefined' "$out/$cc-$name.log" | cut -c1-200)"
      continue
    fi
    res=$("$out/$cc-$name" 2>&1 | head -3 | tr '\n' ' ')
    echo "[$cc] $name: run: ${res:-<no output>}${plain:+ | plain std: $plain}${calls:+ | out-of-line: $calls}"
  done
done
