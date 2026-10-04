// [allocator.adaptor.members]/9: scoped_allocator_adaptor::construct(p, args...) applies
// uses_allocator_construction_args<T>(inner_allocator(), args...), so for T a pair every form
// of [allocator.uses.construction] is accepted, including /17-18 (a pair-like argument such
// as tuple<const char*, const char*> or array<const char*, 2>, P2165) and /19-22 (an object
// that converts to the pair); each member then receives inner_allocator().
#include <scoped_allocator>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include "check.hpp"

template <class T>
struct A {
  using value_type = T;
  int id = 0;
  A() = default;
  explicit A(int i) : id(i) {}
  template <class U>
  A(const A<U>& o) : id(o.id) {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  template <class U>
  friend bool operator==(const A& a, const A<U>& b) { return a.id == b.id; }
};

using Str = std::basic_string<char, std::char_traits<char>, A<char>>;
using P = std::pair<Str, Str>;
static const char* const L1 = "first, long enough not to fit any small-string buffer 0123456789";
static const char* const L2 = "second, long enough not to fit any small-string buffer 0123456789";

struct ToP {
  operator P() const { return P(Str(L1), Str(L2)); }  // members with A<char>(0)
};

int main() {
  std::scoped_allocator_adaptor<A<P>, A<char>> s(A<P>(1), A<char>(2));
  P* p = s.allocate(1);
  auto ok = [&] { return p->first.get_allocator().id == 2 && p->second.get_allocator().id == 2 && p->first == L1 && p->second == L2; };
  s.construct(p, std::tuple<const char*, const char*>(L1, L2));
  CHECK(ok());
  s.destroy(p);
  std::array<const char*, 2> arr{L1, L2};
  s.construct(p, arr);
  CHECK(ok());
  s.destroy(p);
  s.construct(p, ToP{});
  CHECK(ok());
  s.destroy(p);
  s.construct(p, std::pair<const char*, const char*>(L1, L2));
  CHECK(ok());
  s.destroy(p);
  s.deallocate(p, 1);
  return 0;
}
