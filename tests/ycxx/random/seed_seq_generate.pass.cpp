// [rand.util.seedseq]/9: seed_seq::generate fills [begin, end) by the stated algorithm
// (initialisation to 0x8b8b8b8b, t/p/q from n, m = max(s + 1, n) mixing steps with 1664525 and
// T(x) = x xor (x rshift 27), then n steps with 1566083941), all modulo 2^32. "Does nothing if
// begin == end." The result depends only on v, so repeated calls give the same values.
#include <random>
#include <array>
#include <cstdint>
#include "check.hpp"
#include "seed_seq_reference.hpp"

constexpr std::size_t sizes[] = {1, 2, 3, 4, 6, 7, 8, 38, 39, 40, 67, 68, 69, 100, 622, 623, 624, 1000};

void check_input(const std::uint32_t* v, std::size_t s) {
  std::seed_seq q(v, v + s);
  CHECK(q.size() == s);
  static std::uint32_t got[1000], want[1000];
  for (std::size_t n : sizes) {
    q.generate(got, got + n);
    ref_seed_generate(v, s, want, n);
    for (std::size_t i = 0; i < n; ++i) CHECK(got[i] == want[i]);
  }
  // Repeated calls give the same result.
  std::uint32_t a[50], b[50];
  q.generate(a, a + 50);
  q.generate(b, b + 50);
  for (int i = 0; i < 50; ++i) CHECK(a[i] == b[i]);
  // A wider destination type receives the same 32-bit values.
  std::uint64_t wide[70];
  q.generate(wide, wide + 70);
  ref_seed_generate(v, s, want, 70);
  for (int i = 0; i < 70; ++i) CHECK(wide[i] == want[i]);
}

int main() {
  // cppreference's example: seed_seq{1, 2, 3, 4, 5}.generate of 10 values.
  std::seed_seq seq{1, 2, 3, 4, 5};
  std::uint32_t out[10];
  seq.generate(out, out + 10);
  const std::uint32_t expect[10] = {4204997637u, 4246533866u, 1856049002u, 1129615051u, 690460811u,
                                    1075771511u, 46783058u,   3904109078u, 1534123438u, 1495905678u};
  for (int i = 0; i < 10; ++i) CHECK(out[i] == expect[i]);

  static std::uint32_t v[700];
  for (std::uint32_t i = 0; i < 700; ++i) v[i] = i * 2654435761u + 12345u;
  check_input(v, 0);
  check_input(v, 1);
  check_input(v, 5);
  check_input(v, 39);
  check_input(v, 100);
  check_input(v, 623);
  check_input(v, 700);

  // The default-constructed seed_seq (s = 0).
  std::seed_seq empty;
  std::uint32_t e1[16], e2[16];
  empty.generate(e1, e1 + 16);
  ref_seed_generate(nullptr, 0, e2, 16);
  for (int i = 0; i < 16; ++i) CHECK(e1[i] == e2[i]);

  // Empty range: nothing is written.
  std::uint32_t untouched[2] = {7, 9};
  seq.generate(untouched, untouched);
  CHECK(untouched[0] == 7 && untouched[1] == 9);

  // Random-access iterators other than pointers.
  std::array<std::uint32_t, 12> arr{};
  seq.generate(arr.begin(), arr.end());
  std::uint32_t ref12[12];
  const std::uint32_t in[5] = {1, 2, 3, 4, 5};
  ref_seed_generate(in, 5, ref12, 12);
  for (int i = 0; i < 12; ++i) CHECK(arr[i] == ref12[i]);
}
