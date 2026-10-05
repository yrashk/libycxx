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
  [ -f "${ycxx__conf}" ] || return 0
  while IFS='=' read -r ycxx__k ycxx__v; do
    case ${ycxx__k} in
      YCXX_[A-Z_]*)
        if eval "[ -z \"\${${ycxx__k}:-}\" ]"; then
          eval "${ycxx__k}=\${ycxx__v}"
          export "${ycxx__k}"
        fi ;;
    esac
  done <"${ycxx__conf}"
  # macOS: the SDK the compilers were provisioned against (as activate.sh does).
  if [ -z "${SDKROOT:-}" ] && [ -n "${YCXX_SDKROOT:-}" ]; then SDKROOT=$YCXX_SDKROOT; export SDKROOT; fi
  ycxx_env_file=${ycxx__conf}
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

# ycxx_suite_dirs: where the external suites are. Sets ycxx_libcxx_tests and
# ycxx_libstdcxx_tests: $LIBCXX_TESTS / $LIBSTDCXX_TESTS when set; else /opt/src (this project's
# containers and CI); else the cache that tools/fetch-suites fills ($YCXX_SUITES, default
# ~/.local/share/ycxx/suites).
ycxx_suites_cache() { echo "${YCXX_SUITES:-${XDG_DATA_HOME:-$HOME/.local/share}/ycxx/suites}"; }
ycxx_suite_dirs() {
  ycxx__cache=$(ycxx_suites_cache)
  if [ -n "${LIBCXX_TESTS:-}" ]; then ycxx_libcxx_tests=$LIBCXX_TESTS
  elif [ -d /opt/src/llvm-project/libcxx/test ]; then ycxx_libcxx_tests=/opt/src/llvm-project/libcxx/test
  else ycxx_libcxx_tests=$ycxx__cache/llvm-project/libcxx/test; fi
  if [ -n "${LIBSTDCXX_TESTS:-}" ]; then ycxx_libstdcxx_tests=$LIBSTDCXX_TESTS
  elif [ -d /opt/src/libstdcxx-testsuite ]; then ycxx_libstdcxx_tests=/opt/src/libstdcxx-testsuite
  else ycxx_libstdcxx_tests=$ycxx__cache/libstdcxx-testsuite; fi
}

# ycxx_run_config: the configuration of a suite run, from SANITIZER, YCXX_HARDENED=1 and
# YCXX_CXXFLAGS / YCXX_CONFIG_NAME (the last three: own suite only). Sets ycxx_config_name (the
# name of the extra flags: $YCXX_CONFIG_NAME, else made from the flags, "-fno-exceptions -O2" ->
# "fno-exceptions-O2") and ycxx_run_suffix, what run names, logs and baseline files carry after
# the compiler: [-<sanitizers>][-hardened][-<config name>]. Returns 1, with a message, on a name
# that cannot be part of a file name.
ycxx_run_config() {
  ycxx_config_name=${YCXX_CONFIG_NAME:-}
  if [ -z "${ycxx_config_name}" ] && [ -n "${YCXX_CXXFLAGS:-}" ]; then
    ycxx_config_name=$(printf '%s' "$YCXX_CXXFLAGS" | tr -c 'A-Za-z0-9=._' '-' | tr -s '-' | sed 's/^-//; s/-$//')
  fi
  case ${ycxx_config_name} in
    *[!A-Za-z0-9=._-]*|-*)
      echo "error: configuration name '${ycxx_config_name}': use letters, digits, '.', '_', '=' and '-'" >&2
      return 1 ;;
  esac
  ycxx_run_suffix=${SANITIZER:+-$(echo "$SANITIZER" | tr , -)}
  [ "${YCXX_HARDENED:-0}" = 1 ] && ycxx_run_suffix=${ycxx_run_suffix}-hardened
  ycxx_run_suffix=${ycxx_run_suffix}${ycxx_config_name:+-${ycxx_config_name}}
  return 0
}
