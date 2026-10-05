// operator+ and += where both operands are (or point into) the same string, for every length
// from 0 to 80 (across any small-string boundary) and with capacity that is shrunk, exactly
// enough for the result, or one short of it. [string.op.plus]: operator+(const&, const&) and
// (const&, const charT*) are "r = lhs; r.append(rhs); return r;" (/1); (&&, const&) and
// (&&, const charT*) are "lhs.append(rhs); return std::move(lhs);" (/2); (&&, &&) is the same
// "except that both lhs and rhs are left in valid but unspecified states" (/3); (const&, &&)
// and (const charT*, &&) are "rhs.insert(0, lhs); return std::move(rhs);" (/4); the charT
// forms likewise (/6-/8). [string.op.append]: operator+=(str/s/c) is append. Nothing forbids
// lhs and rhs denoting the same object or rhs pointing into lhs, so each result is the
// concatenation of the operands' values before the call.
#include <string>
#include <utility>
#include <cstddef>
#include "check.hpp"

template <class Ch>
static std::basic_string<Ch> make(std::size_t len, int cap_mode, std::size_t want) {
  std::basic_string<Ch> s;
  for (std::size_t i = 0; i < len; ++i) s.push_back(static_cast<Ch>('A' + i % 26));
  if (cap_mode == 0) s.shrink_to_fit();
  else if (cap_mode == 1) s.reserve(want);
  else if (want > 0) s.reserve(want - 1);
  return s;
}

template <class Ch>
static void run() {
  using S = std::basic_string<Ch>;
  for (std::size_t len = 0; len <= 80; ++len) {
    for (int cm = 0; cm < 3; ++cm) {
      const S v = make<Ch>(len, 0, 0);
      const S vv = v + S(v);  // computed from two distinct objects
      auto fresh = [&] { return make<Ch>(len, cm, 2 * len); };
      {
        S s = fresh();
        CHECK(s + s == vv);
        CHECK(s == v);
      }
      {
        S s = fresh();
        S r = std::move(s) + s;
        CHECK(r == vv);
      }
      {
        S s = fresh();
        S r = s + std::move(s);
        CHECK(r == vv);
      }
      {
        S s = fresh();
        S r = std::move(s) + std::move(s);
        CHECK(r == vv);
      }
      {
        S s = fresh();
        CHECK(s + s.c_str() == vv);
        S r = std::move(s) + s.c_str();
        CHECK(r == vv);
      }
      {
        S s = fresh();
        S r = s.c_str() + std::move(s);
        CHECK(r == vv);
      }
      {
        S s = fresh();
        CHECK(s.c_str() + s == vv);
      }
      if (len > 0) {
        S s = fresh();
        S r = std::move(s) + s[len - 1];
        CHECK(r == v + v[len - 1]);
        S t = fresh();
        S q = t[0] + std::move(t);
        CHECK(q == v[0] + v);
        S u = fresh();
        S w = std::move(u) + (u.c_str() + len / 2);
        CHECK(w == v + v.substr(len / 2));
        S y = fresh();
        S z = (y.c_str() + len / 2) + std::move(y);
        CHECK(z == v.substr(len / 2) + v);
      }
      {
        S s = fresh();
        s += s;
        CHECK(s == vv);
        s = fresh();
        s += s.c_str();
        CHECK(s == vv);
        if (len > 0) {
          s = fresh();
          s += s[len / 2];
          CHECK(s == v + v[len / 2]);
          s = fresh();
          s += s.c_str() + 1;
          CHECK(s == v + v.substr(1));
        }
        s = fresh();
        s += std::basic_string_view<Ch>(s);
        CHECK(s == vv);
      }
    }
  }
}

int main() {
  run<char>();
  run<wchar_t>();
  run<char16_t>();
  run<char32_t>();
  run<char8_t>();
  return 0;
}
