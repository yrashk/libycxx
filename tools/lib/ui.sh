# libycxx test scripts: terminal output helpers (POSIX sh; Linux and macOS). Source it:
#   . "$repo/tools/lib/ui.sh"
#
# Colour: on when stdout is a terminal or under GitHub Actions; YCXX_COLOR=always|never|auto
# overrides, NO_COLOR (https://no-color.org) turns it off. Symbols are Unicode when the locale is
# UTF-8, ASCII otherwise.
#
#   ui_title TEXT                  banner for a whole run
#   ui_section TEXT                heading for a group of steps
#   ui_cmd CMD...                  echo a command as it will be run ("$ cmd", dimmed)
#   ui_ok / ui_fail / ui_skip / ui_info / ui_warn LABEL [DETAIL]
#                                  one result line; ui_fail sets ui_failed=1
#   ui_step LABEL LOG CMD...       run CMD with its output going to LOG, showing the command, a
#                                  live elapsed time and the log's latest line on a terminal,
#                                  then a result line with the duration; on failure the end of
#                                  LOG (all of it under GitHub Actions). Returns CMD's status.
#   ui_duration SECONDS            "1m05s"
#   ui_catch_interrupt             Ctrl-C (SIGINT) and SIGTERM set ui_interrupted=1 instead of
#                                  ending the script, which then finishes what it was doing (the
#                                  interrupted command, its logs and reports) and stops early;
#                                  ui_step keeps that trap
# YCXX_VERBOSE=1: ui_step streams CMD's output instead (still saved to LOG).

ui_failed=0
ui_interrupted=0
ui__catching=0

ui_catch_interrupt() {
  ui__catching=1
  trap 'ui_interrupted=1' INT TERM
}

ui__colour=0
case ${YCXX_COLOR:-auto} in
  always) ui__colour=1 ;;
  never) ;;
  *) if [ -z "${NO_COLOR:-}" ] && { [ -t 1 ] || [ "${GITHUB_ACTIONS:-}" = true ] ||
       [ -n "${FORCE_COLOR:-}" ]; }; then ui__colour=1; fi ;;
esac
ui__tty=0
[ -t 1 ] && [ "${TERM:-dumb}" != dumb ] && ui__tty=1

if [ ${ui__colour} = 1 ]; then
  ui_red=$(printf '\033[31m') ui_green=$(printf '\033[32m') ui_yellow=$(printf '\033[33m')
  ui_blue=$(printf '\033[34m') ui_magenta=$(printf '\033[35m') ui_cyan=$(printf '\033[36m')
  ui_bold=$(printf '\033[1m') ui_dim=$(printf '\033[2m') ui_reset=$(printf '\033[0m')
else
  ui_red= ui_green= ui_yellow= ui_blue= ui_magenta= ui_cyan= ui_bold= ui_dim= ui_reset=
fi
case "${LC_ALL:-${LC_CTYPE:-${LANG:-}}}" in
  *UTF-8*|*utf-8*|*UTF8*|*utf8*)
    ui_sym_ok='✔' ui_sym_fail='✘' ui_sym_skip='⊘' ui_sym_warn='!' ui_sym_run='▶' ui_sym_info='•'
    ui__spin='⠋ ⠙ ⠹ ⠸ ⠼ ⠴ ⠦ ⠧ ⠇ ⠏' ;;
  *)
    ui_sym_ok='+' ui_sym_fail='x' ui_sym_skip='-' ui_sym_warn='!' ui_sym_run='>' ui_sym_info='*'
    ui__spin='| / - \' ;;
esac

ui_cols() {
  c=${COLUMNS:-}
  [ -n "$c" ] || c=$(stty size 2>/dev/null </dev/tty | cut -d' ' -f2)
  [ -n "$c" ] && [ "$c" -gt 20 ] 2>/dev/null || c=80
  echo "$c"
}

ui_duration() {
  if [ "$1" -ge 60 ]; then printf '%dm%02ds' $(($1 / 60)) $(($1 % 60)); else printf '%ds' "$1"; fi
}

ui_title() {
  printf '\n%s%s== %s ==%s\n' "${ui_bold}" "${ui_magenta}" "$1" "${ui_reset}"
}
ui_section() {
  printf '\n%s%s%s %s%s\n' "${ui_bold}" "${ui_blue}" "${ui_sym_run}" "$1" "${ui_reset}"
}
# ARGS... quoted for a shell, so that an echoed command can be pasted back.
ui_quote() {
  ui__q=
  for ui__a in "$@"; do
    case ${ui__a} in
      ''|*[!A-Za-z0-9_./=:,+@%-]*) ui__a="'$(printf '%s' "${ui__a}" | sed "s/'/'\\\\''/g")'" ;;
    esac
    ui__q="${ui__q}${ui__q:+ }${ui__a}"
  done
  printf '%s' "${ui__q}"
}
ui_cmd() {
  printf '  %s$ %s%s\n' "${ui_dim}" "$(ui_quote "$@")" "${ui_reset}"
}
ui_ok() {
  printf '  %s%s%s %s%s\n' "${ui_green}" "${ui_sym_ok}" "${ui_reset}" "$1" "${2:+ ${ui_dim}$2${ui_reset}}"
}
ui_fail() {
  printf '  %s%s %s%s%s\n' "${ui_red}${ui_bold}" "${ui_sym_fail}" "$1" "${ui_reset}" "${2:+ ${ui_red}$2${ui_reset}}"
  ui_failed=1
}
ui_skip() {
  printf '  %s%s %s%s%s\n' "${ui_yellow}" "${ui_sym_skip}" "$1" "${ui_reset}" "${2:+ ${ui_dim}$2${ui_reset}}"
}
ui_warn() {
  printf '  %s%s %s%s%s\n' "${ui_yellow}${ui_bold}" "${ui_sym_warn}" "$1" "${ui_reset}" "${2:+ ${ui_yellow}$2${ui_reset}}"
}
ui_info() {
  printf '  %s%s%s %s%s\n' "${ui_cyan}" "${ui_sym_info}" "${ui_reset}" "$1" "${2:+ ${ui_dim}$2${ui_reset}}"
}

# The end of a failed step's log: all of it, folded, under GitHub Actions (the runner's files are
# gone afterwards); the last 40 lines elsewhere.
ui_show_log() {
  if [ "${GITHUB_ACTIONS:-}" = true ]; then
    echo "::group::$2"
    cat "$1"
    echo "::endgroup::"
  else
    tail -n 40 "$1" | sed "s/^/    ${ui_dim}│${ui_reset} /"
    printf '    %sfull log: %s%s\n' "${ui_dim}" "$1" "${ui_reset}"
  fi
}

ui_step() {
  ui__label=$1 ui__log=$2
  shift 2
  mkdir -p "$(dirname "${ui__log}")"
  ui_cmd "$@"
  ui__t0=$(date +%s)
  ui__st=0
  if [ "${YCXX_VERBOSE:-0}" = 1 ]; then
    ui__sf=${ui__log}.status
    { set +e; "$@" 2>&1; echo $? >"${ui__sf}"; } | tee "${ui__log}" | sed "s/^/    ${ui_dim}│${ui_reset} /"
    ui__st=$(cat "${ui__sf}"); rm -f "${ui__sf}"
  elif [ ${ui__tty} = 1 ]; then
    "$@" >"${ui__log}" 2>&1 &
    ui__pid=$!
    # A background command of a non-interactive shell ignores SIGINT: Ctrl-C stops it here.
    trap 'ui_interrupted=1; kill ${ui__pid} 2>/dev/null' INT TERM
    ui__w=$(ui_cols)
    set -- ${ui__spin}
    while kill -0 ${ui__pid} 2>/dev/null; do
      ui__s=$1; shift; [ $# -gt 0 ] || set -- ${ui__spin}
      ui__el=$(ui_duration $(($(date +%s) - ui__t0)))
      ui__head="  ${ui__s} ${ui__label} (${ui__el})"
      ui__room=$((ui__w - ${#ui__head} - 3))
      ui__last=
      [ ${ui__room} -gt 10 ] &&
        ui__last=$(tail -n 1 "${ui__log}" 2>/dev/null | tr -d '\r' | tr '\t' ' ' | cut -c1-${ui__room})
      printf '\r\033[K  %s%s%s %s %s(%s)  %s%s' "${ui_cyan}" "${ui__s}" "${ui_reset}" "${ui__label}" "${ui_dim}" \
        "${ui__el}" "${ui__last}" "${ui_reset}"
      sleep 0.2
    done
    wait ${ui__pid} || ui__st=$?
    if [ ${ui__catching} = 1 ]; then trap 'ui_interrupted=1' INT TERM; else trap - INT TERM; fi
    printf '\r\033[K'
  else
    "$@" >"${ui__log}" 2>&1 || ui__st=$?
  fi
  ui__el=$(ui_duration $(($(date +%s) - ui__t0)))
  if [ "${ui__st}" = 0 ]; then
    ui_ok "${ui__label}" "(${ui__el})"
  elif [ "${ui_interrupted}" = 1 ]; then
    ui_skip "${ui__label}" "(interrupted after ${ui__el})"
    # A script that does not catch interrupts stops here, as it would have without ui_step.
    [ ${ui__catching} = 1 ] || exit 130
  else
    ui_fail "${ui__label}" "(exit ${ui__st}, ${ui__el})"
    [ "${YCXX_VERBOSE:-0}" = 1 ] || ui_show_log "${ui__log}" "${ui__label}: log"
  fi
  return "${ui__st}"
}
