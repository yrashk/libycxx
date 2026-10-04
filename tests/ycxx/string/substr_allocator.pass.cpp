// [string.substr]/1: substr() const& is "return basic_string(*this, pos, n);" and /2
// substr() && is "return basic_string(std::move(*this), pos, n);". Both use the
// [string.cons]/4 constructors, whose allocator parameter defaults to Allocator(), so the
// result has a value-initialized allocator, not a copy of get_allocator().
#include <string>
#include <utility>
#include "test_allocators.hpp"
#include "check.hpp"

using S = std::basic_string<char, std::char_traits<char>, IdAlloc<char>>;

int main() {
  const S s("0123456789 and enough characters to need dynamic storage", IdAlloc<char>(7));
  S a = s.substr(2, 3);
  CHECK(a == "234");
  CHECK(a.get_allocator().id == 0);
  S b = s.substr();
  CHECK(b == s);
  CHECK(b.get_allocator().id == 0);

  S m = s;  // IdAlloc has no select_on_container_copy_construction: id 7
  CHECK(m.get_allocator().id == 7);
  S c = std::move(m).substr(11, 3);
  CHECK(c == "and");
  CHECK(c.get_allocator().id == 0);
  S m2 = s;
  S d = std::move(m2).substr();
  CHECK(d == s);
  CHECK(d.get_allocator().id == 0);
  return 0;
}
