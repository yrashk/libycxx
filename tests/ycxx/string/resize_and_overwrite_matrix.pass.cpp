// resize_and_overwrite for every old size o and requested size n from 0 to 72 (crossing any
// small-string boundary in both directions), every kind of result r (0, about n/2, n) and with
// shrunk or reserved capacity. [string.capacity]/7: k = min(o, n), "this->compare(0, k, p, k)
// == 0 is true before the call", m == n; /10: "replaces the contents of *this with [p, p + r)";
// [basic.string.general]/3 (null terminator after the contents). The string must remain fully
// usable afterwards (append, copy, find).
#include <string>
#include <cstddef>
#include "check.hpp"

template <class Ch>
static Ch ch(std::size_t i, int salt) {
  return static_cast<Ch>('a' + (i * 7 + static_cast<std::size_t>(salt)) % 26);
}

template <class Ch>
static void run() {
  using S = std::basic_string<Ch>;
  for (std::size_t o = 0; o <= 72; o += (o < 30 ? 1 : 3)) {
    for (std::size_t n = 0; n <= 72; n += (n < 30 ? 1 : 5)) {
      for (int rk = 0; rk < 3; ++rk) {
        for (int cap = 0; cap < 2; ++cap) {
          S s;
          for (std::size_t i = 0; i < o; ++i) s.push_back(ch<Ch>(i, 0));
          if (cap) s.reserve(o + 40);
          else s.shrink_to_fit();
          const std::size_t k = o < n ? o : n;
          const std::size_t r = rk == 0 ? 0 : rk == 1 ? n / 2 : n;
          bool old_ok = true, m_ok = true;
          s.resize_and_overwrite(n, [&](Ch* p, std::size_t m) {
            m_ok = m == n;
            for (std::size_t i = 0; i < k; ++i) old_ok = old_ok && p[i] == ch<Ch>(i, 0);
            for (std::size_t i = k; i < n; ++i) p[i] = ch<Ch>(i, 1);
            return r;
          });
          CHECK(old_ok && m_ok);
          CHECK(s.size() == r);
          for (std::size_t i = 0; i < r; ++i) CHECK(s[i] == (i < k ? ch<Ch>(i, 0) : ch<Ch>(i, 1)));
          CHECK(s.c_str()[r] == Ch());
          CHECK(s.capacity() >= s.size());
          const S copy = s;
          s.append(3, Ch('!'));
          CHECK(s.size() == r + 3 && s.substr(0, r) == copy && s.find(Ch('!')) == r);
          CHECK(s.c_str()[r + 3] == Ch());
        }
      }
    }
  }
}

int main() {
  run<char>();
  run<wchar_t>();
  run<char32_t>();
  return 0;
}
