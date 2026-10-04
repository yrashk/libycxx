// [format.functions]/19-/23: format_to_n with M = clamp(n, 0, N) "Places the first M
// characters ... into the range [out, out + M)" and returns {out + M, N}; for a negative n,
// M is 0 and nothing is written.
#include <format>
#include <string>
#include "check.hpp"

int main() {
  char out[4] = {'.', '.', '.', '.'};
  auto r = std::format_to_n(out, -5, "{}", 99);
  CHECK(r.out == out && r.size == 2 && out[0] == '.' && out[1] == '.');
  wchar_t wout[2] = {L'.', L'.'};
  auto wr = std::format_to_n(wout, -1, L"{}", 7);
  CHECK(wr.out == wout && wr.size == 1 && wout[0] == L'.');
}
