// [string.require]/4: references, pointers and iterators to the elements of a basic_string
// may be invalidated only by passing it as a non-const reference argument to a standard
// library function, or by "Calling non-const member functions, except operator[], at, data,
// front, back, begin, rbegin, end, and rend." So const members and those nine non-const
// members keep every pointer valid and pointing at the same characters ([container.reqmts]
// /67: they also do not change the values). c_str() / data() return to_address(begin())
// ([string.accessors]/1), so after any modification a fresh c_str() points at the current,
// null-terminated contents ([basic.string.general]/3).
#include <string>
#include <string_view>
#include <cstring>
#include "check.hpp"

template <class S>
constexpr bool non_invalidating(S s) {
  using C = typename S::value_type;
  const S& cs = s;
  const C* p0 = s.data();
  const C* c = s.c_str();
  auto it = s.begin() + 1;
  (void)s[0]; (void)s[s.size()]; (void)s.at(1); (void)s.data(); (void)s.front(); (void)s.back();
  (void)s.begin(); (void)s.end(); (void)s.rbegin(); (void)s.rend();
  (void)cs.cbegin(); (void)cs.size(); (void)cs.length(); (void)cs.capacity(); (void)cs.empty();
  (void)cs.max_size(); (void)cs.c_str(); (void)cs.find(C('x')); (void)cs.rfind(C('a'));
  (void)cs.compare(cs); (void)cs.substr(1, 2); (void)cs.starts_with(C('a')); (void)cs.contains(C('q'));
  (void)cs.get_allocator(); (void)std::basic_string_view<C>(cs); (void)(cs == cs);
  s[1] = C('Q');  // writing through the element accessors is allowed
  s.at(2) = C('R');
  s.front() = C('P');
  s.back() = C('Z');
  *s.rbegin() = C('Y');
  if (s.data() != p0 || s.c_str() != c || &*it != p0 + 1) return false;
  if (*it != C('Q') || s[0] != C('P') || s[2] != C('R') || s.back() != C('Y')) return false;
  return s[s.size()] == C();
}

constexpr bool fresh_c_str_after_modification() {
  std::string s = "abc";
  for (int i = 0; i < 100; ++i) {
    s.append("xyz");
    const char* p = s.c_str();
    if (p[s.size()] != '\0' || p[0] != 'a' || p[s.size() - 1] != 'z') return false;
  }
  s.erase(3);
  if (std::string_view(s.c_str()) != "abc") return false;
  s.insert(0, "<<");
  s.replace(1, 1, "[[[");
  if (std::string_view(s.c_str()) != "<[[[abc") return false;
  s.resize(2);
  if (s.c_str()[2] != '\0') return false;
  s.shrink_to_fit();
  if (std::string_view(s.c_str()) != "<[") return false;
  return true;
}

static_assert(non_invalidating(std::string("abcdef")));
static_assert(non_invalidating(std::string("a string long enough to be allocated outside any small buffer")));
static_assert(fresh_c_str_after_modification());

int main() {
  CHECK(non_invalidating(std::string("abcdef")));
  CHECK(non_invalidating(std::string("a string long enough to be allocated outside any small buffer")));
  CHECK(non_invalidating(std::wstring(L"abcdef")));
  CHECK(non_invalidating(std::u32string(U"a string long enough to be allocated outside any small buffer")));
  CHECK(fresh_c_str_after_modification());
  std::string s = "hello";
  CHECK(std::strlen(s.c_str()) == 5);
  return 0;
}
