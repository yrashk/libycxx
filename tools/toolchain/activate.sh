# Load libycxx's toolchains into the current bash or zsh session:
#
#   source tools/toolchain/activate.sh [--provision] [--use gcc|clang|DIR]
#
# Reads <toolchains>/toolchains.env written by tools/toolchain/provision (running it in
# --detect-only mode first if the file is missing; with --provision it may also download and
# build what is missing). It then exports:
#   YCXX_ROOT                 the libycxx checkout
#   YCXX_GCC, YCXX_GXX        GCC 16 drivers; YCXX_GCC_INSTALL_DIR its install directory
#   YCXX_CLANG, YCXX_CLANGXX  Clang 23 drivers; YCXX_LLD, YCXX_LLVM_AR
#   SDKROOT                   macOS only, the SDK path (unless already set)
#   PATH                      the toolchain bin directories and $YCXX_ROOT/tools prepended
# With --use, also builds everything compiled in this shell against libycxx
# (docs/BUILDING_PROJECTS.md): gcc or clang names this checkout's build (build/<compiler>), DIR a
# libycxx build tree or installation prefix (the one holding bin/ycxx-c++). It exports
#   CXX, CC                   DIR/bin/ycxx-c++ and DIR/bin/ycxx-cc: libycxx's compilers, for make,
#                             autotools, Meson, CMake (and the sub-builds they start)
#   CMAKE_TOOLCHAIN_FILE      DIR's toolchain.cmake, which CMake (>= 3.21) reads from the environment
#   PKG_CONFIG_PATH           DIR's pkgconfig directory prepended (libycxx.pc)
#   YCXX_USE                  DIR
#   PATH                      DIR/bin prepended too
# and defines `ycxx-unload`, which restores everything it changed and removes itself.
# (The fish version is activate.fish.)

if [ -n "${ZSH_VERSION:-}" ]; then
  _ycxx_here=${${(%):-%x}:A:h}
else
  _ycxx_here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
fi
_ycxx_root=$(cd "$_ycxx_here/../.." && pwd)
_ycxx_conf=${YCXX_TOOLCHAINS:-${XDG_DATA_HOME:-$HOME/.local/share}/ycxx/toolchains}/toolchains.env

_ycxx_provision= _ycxx_use=
while [ $# -gt 0 ]; do
  case $1 in
    --provision) _ycxx_provision=1 ;;
    --use) _ycxx_use=${2:-}; [ $# -gt 1 ] && shift ;;
    --use=*) _ycxx_use=${1#--use=} ;;
    *) echo "activate: unknown argument '$1' (--provision, --use gcc|clang|DIR)" >&2; return 2 ;;
  esac
  shift
done
# --use: the libycxx build or installation, with its compiler wrapper.
_ycxx_use_tc= _ycxx_use_pc=
if [ -n "$_ycxx_use" ]; then
  case $_ycxx_use in gcc|clang) _ycxx_use=$_ycxx_root/build/$_ycxx_use ;; esac
  if [ ! -x "$_ycxx_use/bin/ycxx-c++" ]; then
    echo "activate: --use: no bin/ycxx-c++ in $_ycxx_use (a libycxx build tree, or the prefix of 'cmake --install')" >&2
    return 1
  fi
  _ycxx_use=$(cd "$_ycxx_use" && pwd)
  # (No globs: zsh stops on one that matches nothing.)
  for _ycxx_v in "$_ycxx_use/toolchain.cmake" "$_ycxx_use/lib/cmake/libycxx/toolchain.cmake" \
                 "$_ycxx_use/lib64/cmake/libycxx/toolchain.cmake"; do
    [ -f "$_ycxx_v" ] && { _ycxx_use_tc=$_ycxx_v; break; }
  done
  for _ycxx_v in "$_ycxx_use/pkgconfig" "$_ycxx_use/lib/pkgconfig" "$_ycxx_use/lib64/pkgconfig"; do
    [ -f "$_ycxx_v/libycxx.pc" ] && { _ycxx_use_pc=$_ycxx_v; break; }
  done
fi

if [ -n "$_ycxx_provision" ]; then
  "$_ycxx_here/provision" || { echo "activate: provisioning failed" >&2; return 1; }
elif [ ! -f "$_ycxx_conf" ]; then
  "$_ycxx_here/provision" --detect-only ||
    echo "activate: some compilers are missing; run 'source $_ycxx_here/activate.sh --provision'" >&2
fi

if [ -f "$_ycxx_conf" ]; then
  # A second activation first undoes the first.
  if typeset -f ycxx-unload >/dev/null 2>&1; then ycxx-unload; fi

  # Remember what we change; ycxx-unload puts it back (set or unset).
  _ycxx_saved_vars="PATH SDKROOT YCXX_SDKROOT YCXX_ROOT YCXX_GCC_BIN YCXX_GCC YCXX_GXX YCXX_GCC_INSTALL_DIR YCXX_CLANG_BIN YCXX_CLANG YCXX_CLANGXX YCXX_LLD YCXX_LLVM_AR"
  [ -n "$_ycxx_use" ] && _ycxx_saved_vars="$_ycxx_saved_vars CXX CC CMAKE_TOOLCHAIN_FILE PKG_CONFIG_PATH YCXX_USE"
  for _ycxx_v in $(echo "$_ycxx_saved_vars"); do
    if eval "[ -n \"\${$_ycxx_v+x}\" ]"; then
      eval "_YCXX_OLD_$_ycxx_v=\${$_ycxx_v}"
    else
      eval "_YCXX_OLD_$_ycxx_v=__ycxx_unset__"
    fi
  done

  while IFS='=' read -r _ycxx_k _ycxx_val; do
    case $_ycxx_k in
      YCXX_*) export "$_ycxx_k=$_ycxx_val" ;;
    esac
  done < "$_ycxx_conf"
  export YCXX_ROOT="$_ycxx_root"
  # macOS: compilers not from Apple find the SDK through SDKROOT.
  if [ -n "${YCXX_SDKROOT:-}" ] && [ -z "${SDKROOT:-}" ]; then export SDKROOT="$YCXX_SDKROOT"; fi
  _ycxx_path="$_ycxx_root/tools"
  [ -n "${YCXX_CLANG_BIN:-}" ] && _ycxx_path="$YCXX_CLANG_BIN:$_ycxx_path"
  [ -n "${YCXX_GCC_BIN:-}" ] && _ycxx_path="$YCXX_GCC_BIN:$_ycxx_path"
  [ -n "$_ycxx_use" ] && _ycxx_path="$_ycxx_use/bin:$_ycxx_path"
  export PATH="$_ycxx_path:$PATH"
  if [ -n "$_ycxx_use" ]; then
    export YCXX_USE="$_ycxx_use" CXX="$_ycxx_use/bin/ycxx-c++" CC="$_ycxx_use/bin/ycxx-cc"
    [ -n "$_ycxx_use_tc" ] && export CMAKE_TOOLCHAIN_FILE="$_ycxx_use_tc"
    [ -n "$_ycxx_use_pc" ] && export PKG_CONFIG_PATH="$_ycxx_use_pc${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
  fi

  ycxx-unload() {
    local v old
    for v in $(echo "$_ycxx_saved_vars"); do
      eval "old=\${_YCXX_OLD_$v}"
      if [ "$old" = __ycxx_unset__ ]; then
        unset "$v"
      else
        export "$v=$old"
      fi
      unset "_YCXX_OLD_$v"
    done
    unset _ycxx_saved_vars
    unset -f ycxx-unload
    hash -r 2>/dev/null || rehash 2>/dev/null || true
  }

  hash -r 2>/dev/null || rehash 2>/dev/null || true
  echo "libycxx toolchains loaded (GCC: ${YCXX_GXX:-none}, Clang: ${YCXX_CLANGXX:-none}); 'ycxx-unload' restores the environment" >&2
  [ -n "$_ycxx_use" ] && echo "building against libycxx: CXX=$CXX, CC=$CC${_ycxx_use_tc:+, CMAKE_TOOLCHAIN_FILE=$_ycxx_use_tc}" >&2
fi

unset _ycxx_here _ycxx_root _ycxx_conf _ycxx_v _ycxx_k _ycxx_val _ycxx_path _ycxx_provision _ycxx_use _ycxx_use_tc _ycxx_use_pc
