# libycxx test scripts: turns lit's output (run with -v) into a live progress display.
# POSIX awk (gawk, mawk, BSD awk). Variables (-v):
#   logfile=FILE       every input line is copied here unchanged
#   tty=1          redraw one status line in place (else: a progress line every 10%)
#   verbose=1      also print every passing test
#   details=N      print the output of the first N failures (default 10; the log has all)
#   cols=N         terminal width
#   red green yellow cyan bold dim reset   escape sequences ("" for no colour)
#   s_ok s_fail s_skip                     result symbols
#
# A result line is "<CODE>: <suite> :: <test> (<n> of <total>)"; with -v lit follows each
# failure with its output between "**** TEST '<name>' FAILED ****" and a line of 20 stars.

function now() { srand(); return srand() }
function dur(s) { return s >= 60 ? sprintf("%dm%02ds", int(s / 60), s % 60) : sprintf("%ds", s) }
function clear_status() { if (drawn) { printf "\r\033[K"; drawn = 0 } }
function counts(   s) {
  s = green s_ok " " npass reset
  if (nfail) s = s "  " red bold s_fail " " nfail reset
  else s = s "  " dim s_fail " 0" reset
  if (nskip) s = s "  " yellow s_skip " " nskip reset
  return s
}
function counts_plain() {
  return s_ok " " npass "  " s_fail " " nfail (nskip ? "  " s_skip " " nskip : "")
}
function progress(n, total, name,   el, eta, head, plain, room) {
  el = now() - t0
  eta = (n > 0 && n < total) ? "  ETA " dur(int(el * (total - n) / n)) : ""
  head = sprintf("[" sprintf("%%%dd", length(total "")) "/%d %3d%%  %s%s]", n, total, int(100 * n / total), dur(el), eta)
  if (tty) {
    plain = "  " head "  " counts_plain() "  "
    room = cols - length(plain) - 1
    if (room < 0) room = 0
    if (length(name) > room) name = room > 1 ? "…" substr(name, length(name) - room + 2) : ""
    printf "\r\033[K  %s%s%s  %s  %s%s%s", cyan, head, reset, counts(), dim, name, reset
    drawn = 1
    fflush()
  } else {
    printf "  %s%s%s  %s\n", cyan, head, reset, counts()
    fflush()
  }
}

BEGIN {
  if (details == "") details = 10
  if (cols == "" || cols < 40) cols = 80
  t0 = now(); decile = 0; drawn = 0; inblock = 0; inlist = 0
  npass = nfail = nskip = 0
}

{ print > logfile }

# Inside the output of a failed test.
inblock {
  if ($0 ~ /^\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*$/) {
    inblock = 0
    if (showblock && blines > 30) printf "    %s│ … %d more lines in the log%s\n", dim, blines - 30, reset
    next
  }
  if (showblock) {
    blines++
    clear_status()
    if (blines <= 30) printf "    %s│%s %s\n", dim, reset, $0
  }
  next
}
/^\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\* TEST '.*' FAILED \*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*$/ {
  inblock = 1; blines = 0
  showblock = verbose || shownblocks < details
  if (showblock) shownblocks++
  else if (!hinted) {
    clear_status()
    printf "    %s(output of further failures only in the log)%s\n", dim, reset
    hinted = 1
  }
  next
}

# A result.
/^[A-Z]+: .* \([0-9]+ of [0-9]+\)$/ {
  code = substr($0, 1, index($0, ":") - 1)
  rest = substr($0, length(code) + 3)
  p = match(rest, / \([0-9]+ of [0-9]+\)$/)
  name = substr(rest, 1, p - 1)
  split(substr(rest, p + 2, length(rest) - p - 2), nt, " of ")
  n = nt[1] + 0; total = nt[2] + 0
  if (code == "PASS" || code == "XFAIL" || code == "FLAKYPASS") {
    npass++
    if (verbose) { clear_status(); printf "  %s%s %-5s%s %s\n", green, s_ok, code, reset, name }
  } else if (code == "UNSUPPORTED" || code == "SKIPPED" || code == "EXCLUDED") {
    nskip++
    if (verbose) { clear_status(); printf "  %s%s %s%s %s\n", yellow, s_skip, code, reset, name }
  } else {
    nfail++
    clear_status()
    printf "  %s%s %s%s %s\n", red bold, s_fail, code, reset, name
  }
  if (tty) progress(n, total, name)
  else if (total > 0 && int(10 * n / total) > decile) { decile = int(10 * n / total); progress(n, total, "") }
  next
}

# Everything else: lit's banner, summary and errors.
{
  clear_status()
  line = $0
  if (line ~ /^-- Testing: /) { printf "  %s%s%s\n", dim, line, reset; next }
  if (line ~ /^(Failed|Unexpectedly Passed|Unresolved|Timed Out) Tests \([0-9]+\):$/) {
    inlist = 1; printf "\n  %s%s%s\n", red bold, line, reset; next
  }
  if (line ~ /^[A-Za-z ]+ Tests \([0-9]+\):$/) { inlist = 2; printf "\n  %s%s%s\n", bold, line, reset; next }
  if (inlist && line ~ /^  /) { printf "  %s%s%s\n", inlist == 1 ? red : dim, line, reset; next }
  inlist = 0
  if (line == "" || line ~ /^\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*$/) next
  if (line ~ /^Total Discovered Tests:/) { printf "\n  %s%s%s\n", bold, line, reset; next }
  if (line ~ /^  (Passed|Expectedly Failed|Flakily Passed)/) { printf "  %s%s%s\n", green, line, reset; next }
  if (line ~ /^  (Failed|Unexpectedly Passed|Unresolved|Timed Out)/) { printf "  %s%s%s\n", red bold, line, reset; next }
  if (line ~ /^  (Unsupported|Skipped|Excluded)/) { printf "  %s%s%s\n", yellow, line, reset; next }
  if (line ~ /^Testing Time:/) { printf "  %s%s%s\n", dim, line, reset; next }
  if (line ~ /[Ee]rror|fatal/) { printf "  %s%s%s\n", red, line, reset; next }
  printf "  %s\n", line
}

END {
  clear_status()
  fflush()
}
