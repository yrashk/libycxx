#!/bin/sh
# Runs the quickstart's commands (the first ```sh block of examples/quickstart/README.md, which
# the website shows too) literally, from an empty directory, against this checkout, and checks
# that the program prints ["import", "modules", "std"] and uses no toolchain C++ library.
#
#   tests/quickstart/run.sh [gcc] [clang]        (default: both) the compiler already installed
#   tests/quickstart/run.sh --provision          Clang downloaded into an empty toolchain cache
#
# Three substitutions point the commands at this checkout instead of GitHub; each must apply:
#   - the files are downloaded from this checkout (file://) instead of raw.githubusercontent.com;
#   - the configure command gets -DFETCHCONTENT_SOURCE_DIR_LIBYCXX=<this checkout>, so FetchContent
#     uses this commit's libycxx (the working tree) instead of cloning GitHub's main;
#   - it also gets, for GCC, the README's documented -DYCXX_COMPILER=gcc; without --provision,
#     -DYCXX_PROVISION=OFF (a missing compiler fails instead of being installed); with
#     --provision, -DYCXX_USE_SYSTEM_COMPILERS=OFF (an installed Clang 23 is not used).
# YCXX_TOOLCHAINS is a fresh, empty directory either way. --provision downloads LLVM's release
# (about 2 GB) and needs about 6 GB of disk. CMAKE_BUILD_PARALLEL_LEVEL limits the build's jobs.
# Logs and timings: $YCXX_QUICKSTART_TEST_DIR (default build/quickstart-test)/<variant>-<cc>/.
set -eu
repo=$(cd "$(dirname "$0")/../.." && pwd)
. "$repo/tools/lib/ui.sh"
. "$repo/tools/lib/env.sh"
work=${YCXX_QUICKSTART_TEST_DIR:-$repo/build/quickstart-test}
readme=$repo/examples/quickstart/README.md
expected='["import", "modules", "std"]'
raw=https://raw.githubusercontent.com/yrashk/libycxx/main/
configure='cmake -B build -G Ninja'

provision=0
compilers=
for a in "$@"; do
  case $a in
    --provision) provision=1 ;;
    gcc|clang) compilers="$compilers $a" ;;
    *) echo "usage: tests/quickstart/run.sh [--provision] [gcc] [clang]" >&2; exit 2 ;;
  esac
done
if [ $provision = 1 ]; then
  [ -z "$compilers" ] || [ "$compilers" = " clang" ] || { echo "--provision: Clang only" >&2; exit 2; }
  compilers=clang variant=provision what="Clang downloaded into an empty cache"
else
  # The compilers tests/cmake/run.sh uses (YCXX_GXX, YCXX_GCC_INSTALL_DIR, ...); --provision runs
  # in a newcomer's environment instead.
  ycxx_env_load
  compilers=${compilers:-gcc clang} variant=installed what="compiler already installed"
fi
fail=0
ok() { ui_ok "[$1] $2"; }
bad() { ui_fail "[$1] $2"; fail=1; }

# The commands as documented.
commands=$(awk '/^```sh$/ { if (!done) on = 1; next } /^```/ { if (on) { on = 0; done = 1 } next } on' "$readme")
if [ -z "$commands" ]; then
  ui_fail "no \`\`\`sh block in $readme"; exit 1
fi
# A line of the commands that is exactly LINE (counted: it must be there once).
has_line() { [ "$(printf '%s\n' "$commands" | grep -cxF "$1")" = 1 ]; }
if ! printf '%s\n' "$commands" | grep -qF "$raw"; then
  ui_fail "the quickstart commands no longer download from $raw; update tests/quickstart/run.sh"; exit 1
fi
if ! has_line "$configure"; then
  ui_fail "the quickstart commands no longer configure with '$configure'; update tests/quickstart/run.sh"; exit 1
fi
if ! grep -qF "$configure -DYCXX_COMPILER=gcc" "$readme"; then
  ui_fail "$readme no longer documents '$configure -DYCXX_COMPILER=gcc'; update tests/quickstart/run.sh"; exit 1
fi

for c in $compilers; do
  ui_section "Quickstart commands with $c, $what (examples/quickstart/README.md)"
  d=$work/$variant-$c
  rm -rf "$d"
  mkdir -p "$d/run"
  log=$d/log.txt
  ui_info "log" "$log"
  extra="-DFETCHCONTENT_SOURCE_DIR_LIBYCXX=$repo"
  [ $c = gcc ] && extra="$extra -DYCXX_COMPILER=gcc"
  if [ $provision = 1 ]; then extra="$extra -DYCXX_USE_SYSTEM_COMPILERS=OFF"
  else extra="$extra -DYCXX_PROVISION=OFF"; fi
  # The repository's path is used literally in the substitutions below.
  case $repo in *'|'*|*'&'*|*'\'*) ui_fail "unsupported characters in $repo"; exit 1 ;; esac
  printf '%s\n' "$commands" |
    sed -e "s|$raw|file://$repo/|g" -e "s|^$configure\$|& $extra|" >"$d/commands.sh"
  printf 'YCXX_TOOLCHAINS=%s, in %s:\n' "$d/toolchains" "$d/run" >>"$log"
  cat "$d/commands.sh" >>"$log"
  printf -- '----\n' >>"$log"
  sed 's/^/    /' "$d/commands.sh"
  start=$(date +%s)
  st=0
  # "Already installed" means reachable on PATH, as for a user: the compiler tools/test uses may
  # live in the default toolchain cache (macOS's provisioned Clang), which this run bypasses.
  path=$PATH
  if [ $provision = 0 ]; then
    if [ $c = gcc ]; then cxx=${YCXX_GXX:-g++-16}; else cxx=${YCXX_CLANGXX:-clang++-23}; fi
    cxx=$(command -v "$cxx" 2>/dev/null || true)
    [ -n "$cxx" ] && path=$(dirname "$cxx"):$PATH
  fi
  (cd "$d/run" && PATH=$path YCXX_TOOLCHAINS=$d/toolchains sh -ex "$d/commands.sh") >>"$log" 2>&1 || st=$?
  secs=$(( $(date +%s) - start ))
  printf '%s %s %s seconds\n' "$variant" "$c" "$secs" >"$d/time.txt"
  last=$(tail -n 1 "$log")
  if [ $st = 0 ] && [ "$last" = "$expected" ]; then
    ok $c "the commands ran and the program printed $expected ($(ui_duration $secs))"
  else
    bad $c "the commands failed (exit $st) or printed something else (see $log)"
    continue
  fi
  exe=$(find "$d/run" -path '*/build/hello' -type f | head -n 1)
  if [ "$(uname -s)" = Darwin ]; then libs=$(otool -L "$exe")
  else libs=$(readelf -d "$exe" | grep NEEDED); fi
  printf '%s\n' "$libs" >>"$log"
  if printf '%s\n' "$libs" | grep -qE 'libstdc\+\+|libc\+\+'; then
    bad $c "the program links the toolchain's C++ library (see $log)"
  else
    ok $c "no libstdc++/libc++"
  fi
  key=YCXX_GXX; [ $c = clang ] && key=YCXX_CLANGXX
  if [ $provision = 1 ]; then
    if grep -q "^$key=$d/toolchains/llvm-" "$d/toolchains/toolchains.env" 2>/dev/null; then
      ok $c "Clang was downloaded into the empty toolchain cache ($(du -sh "$d/toolchains" | cut -f1))"
    else
      bad $c "Clang was not installed into $d/toolchains (see $d/toolchains/toolchains.env)"
    fi
    # The download takes most of the space; the result has been checked.
    rm -rf "$d/toolchains"
  elif grep -q "^$key=/" "$d/toolchains/toolchains.env" 2>/dev/null; then
    ok $c "the installed compiler was found and recorded ($(grep "^$key=" "$d/toolchains/toolchains.env" | cut -d= -f2))"
  else
    bad $c "$key missing from $d/toolchains/toolchains.env"
  fi
done

echo
if [ $fail = 0 ]; then ui_ok "Quickstart: all checks passed"; else ui_fail "Quickstart: failures (logs under $work)"; fi
exit $fail
