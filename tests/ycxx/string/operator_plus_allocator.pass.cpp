// [string.op.plus]: which operand's allocator the result of operator+ carries.
// The const& forms are "basic_string r = lhs;" (or "= rhs;") and so copy-construct, obtaining
// the allocator through select_on_container_copy_construction ([container.reqmts]/64,
// [string.require]/3). The rvalue forms return std::move(lhs) after lhs.append(...) /
// lhs.push_back(...), or std::move(rhs) after rhs.insert(...), so the result keeps that
// operand's allocator; for (lhs&&, rhs&&) it is "lhs.append(rhs); return std::move(lhs);",
// i.e. lhs's allocator even when the allocators differ.
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include "check.hpp"

template <class T>
struct SoccAlloc {
  using value_type = T;
  using propagate_on_container_copy_assignment = std::false_type;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_swap = std::false_type;
  using is_always_equal = std::false_type;
  int id = 0;
  SoccAlloc() = default;
  explicit SoccAlloc(int i) : id(i) {}
  template <class U>
  SoccAlloc(const SoccAlloc<U>& o) : id(o.id) {}
  SoccAlloc select_on_container_copy_construction() const { return SoccAlloc(id + 100); }
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  template <class U>
  friend bool operator==(const SoccAlloc& a, const SoccAlloc<U>& b) { return a.id == b.id; }
};

using S = std::basic_string<char, std::char_traits<char>, SoccAlloc<char>>;

int main() {
  const S l("left", SoccAlloc<char>(1));
  const S r("right", SoccAlloc<char>(2));
  std::string_view sv = "sv";
  // const& on the side that is copied: select_on_container_copy_construction is used.
  {
    S x = l + r;
    CHECK(x == "leftright" && x.get_allocator().id == 101);
    S y = l + "!";
    CHECK(y == "left!" && y.get_allocator().id == 101);
    S z = l + '!';
    CHECK(z == "left!" && z.get_allocator().id == 101);
    S w = "<" + r;
    CHECK(w == "<right" && w.get_allocator().id == 102);
    S v = '<' + r;
    CHECK(v == "<right" && v.get_allocator().id == 102);
    S a = l + sv;
    CHECK(a == "leftsv" && a.get_allocator().id == 101);
    S b = sv + r;
    CHECK(b == "svright" && b.get_allocator().id == 102);
  }
  // rvalue lhs: the result is lhs.
  {
    S m("m", SoccAlloc<char>(3));
    S x = std::move(m) + r;
    CHECK(x == "mright" && x.get_allocator().id == 3);
    S m2("m", SoccAlloc<char>(4));
    S y = std::move(m2) + "!";
    CHECK(y == "m!" && y.get_allocator().id == 4);
    S m3("m", SoccAlloc<char>(5));
    S z = std::move(m3) + '!';
    CHECK(z == "m!" && z.get_allocator().id == 5);
    S m4("m", SoccAlloc<char>(6));
    S a = std::move(m4) + sv;
    CHECK(a == "msv" && a.get_allocator().id == 6);
  }
  // rvalue rhs only: the result is rhs.
  {
    S m("m", SoccAlloc<char>(7));
    S x = l + std::move(m);
    CHECK(x == "leftm" && x.get_allocator().id == 7);
    S m2("m", SoccAlloc<char>(8));
    S y = "<" + std::move(m2);
    CHECK(y == "<m" && y.get_allocator().id == 8);
    S m3("m", SoccAlloc<char>(9));
    S z = '<' + std::move(m3);
    CHECK(z == "<m" && z.get_allocator().id == 9);
    S m4("m", SoccAlloc<char>(10));
    S a = sv + std::move(m4);
    CHECK(a == "svm" && a.get_allocator().id == 10);
  }
  // both rvalues with different allocators: lhs's allocator wins.
  {
    S a("a long left operand that defeats any small buffer", SoccAlloc<char>(11));
    S b("a long right operand that defeats any small buffer", SoccAlloc<char>(12));
    S x = std::move(a) + std::move(b);
    CHECK(x == "a long left operand that defeats any small buffera long right operand that defeats any small buffer");
    CHECK(x.get_allocator().id == 11);
    S c("c", SoccAlloc<char>(13));
    S d("a long right operand that defeats any small buffer", SoccAlloc<char>(14));
    S y = std::move(c) + std::move(d);
    CHECK(y == "ca long right operand that defeats any small buffer");
    CHECK(y.get_allocator().id == 13);
  }
  return 0;
}
