# libycxx test scripts: turns lit's output (run with -a: every test's output) into a live,
# traceable display. POSIX awk (gawk, mawk, BSD awk). Variables (-v):
#   logfile=FILE   every input line is copied here unchanged
#   tty=1          also keep a status line (counts, elapsed time, ETA) redrawn under the results
#   quiet=1        list only the tests that did not pass (plus progress every 10% without a tty)
#   verbose=1      print every test's whole transcript, passing ones included
#   details=N      print the transcript of the first N failures (default 10; the log has all)
#   red green yellow cyan bold dim reset   escape sequences ("" for no colour)
#   s_ok s_fail s_skip                     result symbols
#
# lit prints "<CODE>: <suite> :: <test> (<n> of <total>)" per test, then (with -a) its output,
# for a failure under a "**** TEST '<name>' FAILED ****" header, closed by a line of 20 stars.
# The lit formats (tests/ycxxlit/transcript.py) write each command as "$ <command>" followed by
# "[<step>: exit <status>, <seconds>s]": a test's line shows those steps as its evidence.

function now() { srand(); return srand() }
function dur(s) { return s >= 60 ? sprintf("%dm%02ds", int(s / 60), s % 60) : sprintf("%ds", s) }
function clear_status() { if (drawn) { printf "\r\033[K"; drawn = 0 } }
function counts(   s) {
  s = green s_ok " " npass reset
  s = s "  " (nfail ? red bold : dim) s_fail " " nfail reset
  if (nskip) s = s "  " yellow s_skip " " nskip reset
  return s
}
function head_of(n, tot,   el, eta) {
  el = now() - t0
  eta = (n > 0 && n < tot) ? "  ETA " dur(int(el * (tot - n) / n)) : ""
  return sprintf("[" sprintf("%%%dd", length(tot "")) "/%d %3d%%  %s%s]", n, tot, int(100 * n / tot), dur(el), eta)
}
function status() {
  if (!tty || total == 0) return
  printf "\r\033[K  %s%s%s  %s", cyan, head_of(done, total), reset, counts()
  drawn = 1
  fflush()
}
function decile_progress() {
  if (tty || !quiet || total == 0 || int(10 * done / total) <= decile) return
  decile = int(10 * done / total)
  printf "  %s%s%s  %s\n", cyan, head_of(done, total), reset, counts()
}
# "[compile: exit 0, 0.06s; must fail]" -> "compile exit 0 0.06s (must fail)"
function evidence_of(line,   s, extra, i) {
  s = substr(line, 2, length(line) - 2)
  extra = ""
  i = index(s, "; ")
  if (i) { extra = " (" substr(s, i + 2) ")"; s = substr(s, 1, i - 1) }
  sub(/: /, " ", s)
  sub(/, /, " ", s)
  return s extra
}

# The test whose output is being read: printed once its output has ended.
function begin_test(c, nm, n, tot) {
  code = c; name = nm; done = n; total = tot
  pending = 1; nlines = 0; evidence = ""; reason = ""; firsterr = ""
  is_bad = !(c == "PASS" || c == "XFAIL" || c == "FLAKYPASS" || c == "UNSUPPORTED" || c == "SKIPPED" || c == "EXCLUDED")
  if (c == "PASS" || c == "XFAIL" || c == "FLAKYPASS") npass++
  else if (is_bad) nfail++
  else nskip++
}
function test_line(line,   i) {
  if (line ~ /^\*+ TEST '.*' [A-Z]+ \*+$/) return
  if (line ~ /^\[[a-z]+: (exit -?[0-9]+|TIMEOUT)/ && line ~ /\]$/)
    evidence = evidence (evidence == "" ? "" : " · ") evidence_of(line)
  if (reason == "" && line != "") reason = line
  # A test that must not compile: the compiler's first error shows why it did not.
  if (firsterr == "" && evidence ~ /must fail/ && (i = index(line, "error: ")))
    firsterr = substr(line, i, 110)
  nlines++
  body[nlines] = line
}
function end_test(   sym, col, i, limit, label) {
  if (!pending) return
  pending = 0
  if (is_bad) { sym = s_fail; col = red bold }
  else if (code == "UNSUPPORTED" || code == "SKIPPED" || code == "EXCLUDED") { sym = s_skip; col = yellow }
  else { sym = s_ok; col = green }
  if (!quiet || is_bad) {
    clear_status()
    label = code == "XFAIL" ? "XFAIL" : code
    printf "  %s%s %-11s%s %s%s%s", col, sym, label, reset, (is_bad ? bold : ""), name, reset
    if (evidence != "") {
      printf "  %s%s%s", dim, evidence, reset
      if (firsterr != "" && !is_bad) printf "  %s→ %s%s", dim, firsterr, reset
    } else if (reason != "" && !is_bad) {
      printf "  %s%s%s", dim, reason, reset # e.g. why a test is unsupported
    }
    if (code == "XFAIL") printf "  %s(expected failure)%s", dim, reset
    printf "\n"
  }
  limit = 0
  if (verbose) limit = nlines
  else if (is_bad && shownfail < details) { limit = nlines < 30 ? nlines : 30; shownfail++ }
  else if (is_bad && !hinted) {
    clear_status()
    printf "    %s(the transcripts of further failures are in the log and the report)%s\n", dim, reset
    hinted = 1
  }
  for (i = 1; i <= limit; i++) printf "    %s│%s %s\n", dim, reset, body[i]
  if (limit > 0 && limit < nlines) printf "    %s│ … %d more lines in the log%s\n", dim, nlines - limit, reset
  decile_progress()
  status()
}

BEGIN {
  if (details == "") details = 10
  t0 = now(); decile = 0; drawn = 0; pending = 0; inlist = 0; total = 0; done = 0
  npass = nfail = nskip = 0
}

{ print > logfile }

# A result.
/^[A-Z]+: .* \([0-9]+ of [0-9]+\)$/ {
  end_test()
  c = substr($0, 1, index($0, ":") - 1)
  rest = substr($0, length(c) + 3)
  p = match(rest, / \([0-9]+ of [0-9]+\)$/)
  nm = substr(rest, 1, p - 1)
  i = index(nm, " :: ")
  if (i) nm = substr(nm, i + 4)
  split(substr(rest, p + 2, length(rest) - p - 2), nt, " of ")
  begin_test(c, nm, nt[1] + 0, nt[2] + 0)
  next
}

# The end of a test's output.
pending && /^\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*$/ { end_test(); next }
pending { test_line($0); next }

# Everything else: lit's banner, summary and errors.
{
  clear_status()
  line = $0
  if (line ~ /^\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*$/) next
  if (line ~ /^-- Testing: /) { printf "  %s%s%s\n", dim, line, reset; next }
  if (line ~ /^(Failed|Unexpectedly Passed|Unresolved|Timed Out) Tests \([0-9]+\):$/) {
    inlist = 1; printf "\n  %s%s%s\n", red bold, line, reset; next
  }
  if (line ~ /^[A-Za-z ]+ Tests \([0-9]+\):$/) { inlist = 2; printf "\n  %s%s%s\n", bold, line, reset; next }
  if (inlist && line ~ /^  /) { printf "  %s%s%s\n", inlist == 1 ? red : dim, line, reset; next }
  inlist = 0
  if (line == "") next
  if (line ~ /^Total Discovered Tests:/) { printf "\n  %s%s%s\n", bold, line, reset; next }
  if (line ~ /^  (Passed|Expectedly Failed|Flakily Passed)/) { printf "  %s%s%s\n", green, line, reset; next }
  if (line ~ /^  (Failed|Unexpectedly Passed|Unresolved|Timed Out)/) { printf "  %s%s%s\n", red bold, line, reset; next }
  if (line ~ /^  (Unsupported|Skipped|Excluded)/) { printf "  %s%s%s\n", yellow, line, reset; next }
  if (line ~ /^Testing Time:/) { printf "  %s%s%s\n", dim, line, reset; next }
  if (line ~ /[Ee]rror|fatal/) { printf "  %s%s%s\n", red, line, reset; next }
  printf "  %s\n", line
}

END {
  end_test()
  clear_status()
  fflush()
}
