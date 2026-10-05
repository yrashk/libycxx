// format_to_n for every limit n around the output size, for outputs from empty to several
// thousand characters (so that any internal buffering boundary is crossed), with padding by
// multi-code-unit fill characters, escaped strings, ranges, and the wide forms; through a
// pointer, a back_insert_iterator and a plain output iterator. [format.functions]/19-23:
// "Let N be formatted_size(fmt, args...) and M be clamp(n, 0, N)"; "Effects: Places the
// first M characters of the character representation of the arguments ... into the range
// [out, out + M)"; "Returns: {out + M, N}". /27 formatted_size returns N. No character beyond
// the first M is written.
#include <format>
#include <string>
#include <vector>
#include <list>
#include <iterator>
#include <cstddef>
#include "check.hpp"

// an output iterator that is not a pointer, appending to a vector
template <class Ch>
struct Sink {
  using difference_type = std::ptrdiff_t;
  std::vector<Ch>* v;
  Sink& operator*() { return *this; }
  Sink& operator=(Ch c) {
    v->push_back(c);
    return *this;
  }
  Sink& operator++() { return *this; }
  Sink operator++(int) { return *this; }
};

static std::vector<std::ptrdiff_t> limits(std::ptrdiff_t N) {
  std::vector<std::ptrdiff_t> r = {-1000, -2, -1};
  for (std::ptrdiff_t n = 0; n <= N + 2; ++n)
    if (n < 70 || n > N - 3 || ((n & (n - 1)) == 0) || (((n + 1) & n) == 0) || (((n - 1) & (n - 2)) == 0) || n % 97 == 0)
      r.push_back(n);
  r.push_back(N + 1000);
  return r;
}

template <class Ch, class F>
static void sweep(const std::basic_string<Ch>& full, std::size_t fsize, F f) {
  const auto N = static_cast<std::ptrdiff_t>(full.size());
  CHECK(fsize == full.size());
  std::vector<Ch> buf(full.size() + 16);
  for (std::ptrdiff_t n : limits(N)) {
    const std::ptrdiff_t M = n < 0 ? 0 : n > N ? N : n;
    for (auto& c : buf) c = Ch('#');
    auto r = f(buf.data(), n);
    CHECK(r.size == N);
    CHECK(r.out == buf.data() + M);
    CHECK(std::basic_string<Ch>(buf.data(), buf.data() + M) == full.substr(0, static_cast<std::size_t>(M)));
    for (std::size_t i = static_cast<std::size_t>(M); i < buf.size(); ++i) CHECK(buf[i] == Ch('#'));
    std::basic_string<Ch> s;
    auto r2 = f(std::back_inserter(s), n);
    CHECK(r2.size == N && s == full.substr(0, static_cast<std::size_t>(M)));
    std::vector<Ch> v;
    auto r3 = f(Sink<Ch>{&v}, n);
    CHECK(r3.size == N && std::basic_string<Ch>(v.begin(), v.end()) == full.substr(0, static_cast<std::size_t>(M)));
  }
}

int main() {
  sweep(std::string(), std::formatted_size(""), [](auto out, std::ptrdiff_t n) { return std::format_to_n(out, n, ""); });
  sweep(std::format("{}", 42), std::formatted_size("{}", 42),
        [](auto out, std::ptrdiff_t n) { return std::format_to_n(out, n, "{}", 42); });
  sweep(std::format("a{{b}}c{}d", -7), std::formatted_size("a{{b}}c{}d", -7),
        [](auto out, std::ptrdiff_t n) { return std::format_to_n(out, n, "a{{b}}c{}d", -7); });
  // multi-byte fill: each fill character is two (é), three (中) or four (🙂) code units
  sweep(std::format("{:é^41}|{:中<9}|{:🙂>7}", "x", 12, 3.5), std::formatted_size("{:é^41}|{:中<9}|{:🙂>7}", "x", 12, 3.5),
        [](auto out, std::ptrdiff_t n) { return std::format_to_n(out, n, "{:é^41}|{:中<9}|{:🙂>7}", "x", 12, 3.5); });
  // long outputs crossing internal buffer sizes
  const std::string big(5000, 'q');
  for (int w : {255, 256, 257, 1023, 1024, 1025, 4095, 4096, 4097}) {
    sweep(std::format("{:>{}}", "z", w), std::formatted_size("{:>{}}", "z", w),
          [w](auto out, std::ptrdiff_t n) { return std::format_to_n(out, n, "{:>{}}", "z", w); });
  }
  sweep(std::format("{}{}{}", big, 1.0 / 3, big), std::formatted_size("{}{}{}", big, 1.0 / 3, big),
        [&](auto out, std::ptrdiff_t n) { return std::format_to_n(out, n, "{}{}{}", big, 1.0 / 3, big); });
  // escaped strings and ranges
  const std::vector<std::string> vs = {"a\tb", "\"q\"", "\xc3\xa9", ""};
  sweep(std::format("{::?}", vs), std::formatted_size("{::?}", vs),
        [&](auto out, std::ptrdiff_t n) { return std::format_to_n(out, n, "{::?}", vs); });
  std::vector<int> many(700);
  for (std::size_t i = 0; i < many.size(); ++i) many[i] = static_cast<int>(i * 7919 % 100003) - 50000;
  sweep(std::format("{::+#x}", many), std::formatted_size("{::+#x}", many),
        [&](auto out, std::ptrdiff_t n) { return std::format_to_n(out, n, "{::+#x}", many); });
  // wide
  sweep(std::format(L"{:*^31}{}", L"wide", 2.5), std::formatted_size(L"{:*^31}{}", L"wide", 2.5),
        [](auto out, std::ptrdiff_t n) { return std::format_to_n(out, n, L"{:*^31}{}", L"wide", 2.5); });
  const std::wstring wbig(3000, L'w');
  sweep(std::format(L"{}|{:>1500}", wbig, 1), std::formatted_size(L"{}|{:>1500}", wbig, 1),
        [&](auto out, std::ptrdiff_t n) { return std::format_to_n(out, n, L"{}|{:>1500}", wbig, 1); });
  // a list as the destination of format_to_n (bidirectional, not contiguous)
  std::list<char> l;
  auto lr = std::format_to_n(std::back_inserter(l), 5, "{}", 1234567);
  CHECK(lr.size == 7 && std::string(l.begin(), l.end()) == "12345");
  return 0;
}
