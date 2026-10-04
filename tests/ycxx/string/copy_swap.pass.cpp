// [string.copy]/1: copy(s, n, pos) is basic_string_view(*this).copy(s, n, pos): copies
// min(n, size() - pos) characters, returns that count, does not null-terminate, throws
// out_of_range if pos > size(). [string.swap]: swap exchanges the contents; noexcept when
// propagate_on_container_swap or is_always_equal (an implementation may add noexcept
// elsewhere, [res.on.exception.handling]/5, so only the positive case is checked); constant time. [string.special]:
// non-member swap is lhs.swap(rhs) with noexcept(noexcept(lhs.swap(rhs))).
// [container.reqmts]/64-65: with propagate_on_container_swap the allocators are exchanged.
#include <string>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "test_allocators.hpp"
#include "check.hpp"

static_assert(std::is_nothrow_swappable_v<std::string>);
static_assert(noexcept(std::declval<std::string&>().swap(std::declval<std::string&>())));
using PS = std::basic_string<char, std::char_traits<char>, IdAlloc<char, false, false, true>>;
using NS = std::basic_string<char, std::char_traits<char>, IdAlloc<char, false, false, false>>;
static_assert(noexcept(std::declval<PS&>().swap(std::declval<PS&>())));
static_assert(std::is_nothrow_swappable_v<PS>);

constexpr bool test() {
  const std::string s = "abcdef";
  char buf[8] = {'*', '*', '*', '*', '*', '*', '*', '*'};
  if (s.copy(buf, 3) != 3 || buf[0] != 'a' || buf[2] != 'c' || buf[3] != '*') return false;
  if (s.copy(buf, 100, 4) != 2 || buf[0] != 'e' || buf[1] != 'f' || buf[2] != 'c') return false;
  if (s.copy(buf, 5, 6) != 0) return false;

  std::string a = "short";
  std::string b = "a much longer string that will not fit in any small buffer";
  a.swap(b);
  if (b != "short" || a != "a much longer string that will not fit in any small buffer") return false;
  swap(a, b);
  if (a != "short") return false;
  std::swap(a, b);
  if (b != "short") return false;
  a.swap(a);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  const std::string s = "abc";
  char buf[4];
  bool threw = false;
  try {
    s.copy(buf, 1, 4);
  } catch (const std::out_of_range&) {
    threw = true;
  }
  CHECK(threw);
  {
    PS a("first, long enough to be allocated on the heap", IdAlloc<char, false, false, true>(1));
    PS b("second", IdAlloc<char, false, false, true>(2));
    a.swap(b);
    CHECK(a == "second" && a.get_allocator().id == 2);
    CHECK(b == "first, long enough to be allocated on the heap" && b.get_allocator().id == 1);
  }
  {
    NS a("first, long enough to be allocated on the heap", IdAlloc<char>(1));
    NS b("second", IdAlloc<char>(1));
    swap(a, b);
    CHECK(a == "second" && b == "first, long enough to be allocated on the heap");
    CHECK(a.get_allocator().id == 1 && b.get_allocator().id == 1);
  }
  return 0;
}
