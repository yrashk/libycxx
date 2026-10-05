# libycxx test scripts: toolchain and lit discovery (POSIX sh; Linux and macOS). Source it:
#   . "$repo/tools/lib/env.sh"
#
#   ycxx_env_load    when the YCXX_* compiler variables are not set (no activate.sh in this
#                    shell), take them from the toolchains.env that tools/toolchain/provision
#                    writes; variables already set win
#   ycxx_lit_cmd     sets ycxx_lit to the command that runs lit:
#                      $YCXX_LIT if set (a command line, e.g. "lit" or "python3 -m lit");
#                      else uvx with lit pinned to $YCXX_LIT_VERSION (default: the LLVM release
#                      the libc++ suite comes from), so no install is needed;
#                      else a lit on PATH.
#                    Returns 1, with a hint, when none is available.

ycxx_env_load() {
  [ -n "${YCXX_GXX:-}${YCXX_CLANGXX:-}" ] && return 0
  ycxx__conf=${YCXX_TOOLCHAINS:-${XDG_DATA_HOME:-$HOME/.local/share}/ycxx/toolchains}/toolchains.env
  [ -f "$ycxx__conf" ] || return 0
  while IFS='=' read -r ycxx__k ycxx__v; do
    case $ycxx__k in
      YCXX_[A-Z_]*)
        if eval "[ -z \"\${$ycxx__k:-}\" ]"; then
          eval "$ycxx__k=\$ycxx__v"
          export "$ycxx__k"
        fi ;;
    esac
  done <"$ycxx__conf"
  # macOS: the SDK the compilers were provisioned against (as activate.sh does).
  if [ -z "${SDKROOT:-}" ] && [ -n "${YCXX_SDKROOT:-}" ]; then SDKROOT=$YCXX_SDKROOT; export SDKROOT; fi
  ycxx_env_file=$ycxx__conf
}

ycxx_lit_cmd() {
  if [ -n "${YCXX_LIT:-}" ]; then
    ycxx_lit=$YCXX_LIT
  elif command -v uvx >/dev/null 2>&1; then
    ycxx_lit="uvx --quiet --from lit==${YCXX_LIT_VERSION:-23.1.2} lit"
  elif command -v lit >/dev/null 2>&1; then
    ycxx_lit=lit
  else
    echo "error: lit not found. Install uv (https://docs.astral.sh/uv/: on macOS 'brew install uv'," >&2
    echo "       elsewhere 'curl -LsSf https://astral.sh/uv/install.sh | sh'); lit then runs through" >&2
    echo "       uvx. Or set YCXX_LIT to a lit command." >&2
    return 1
  fi
}
