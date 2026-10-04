// [string.cons]/2-3: copy and move constructors construct an object whose value is that of
// str prior to the call; the move constructor is noexcept and leaves str valid but
// unspecified. [string.cons]/24-25: basic_string(const basic_string&, const Allocator&) and
// basic_string(basic_string&&, const Allocator&): same value, stored allocator constructed
// from alloc; the second form throws nothing if alloc == str.get_allocator().
// [container.reqmts]/64: copy construction obtains the allocator from
// select_on_container_copy_construction; move construction moves the allocator.
#include <string>
#include <type_traits>
#include <utility>
#include "test_allocators.hpp"
#include "check.hpp"

static_assert(std::is_nothrow_move_constructible_v<std::string>);
static_assert(std::is_copy_constructible_v<std::string>);

template <class T>
struct SoccAlloc {
  using value_type = T;
  int id = 0;
  SoccAlloc() = default;
  explicit SoccAlloc(int i) : id(i) {}
  template <class U>
  SoccAlloc(const SoccAlloc<U>& o) : id(o.id) {}
  SoccAlloc select_on_container_copy_construction() const { return SoccAlloc(id + 100); }
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  friend bool operator==(const SoccAlloc& a, const SoccAlloc& b) { return a.id == b.id; }
};

constexpr bool test_values() {
  const char* lens[] = {"", "a", "hello", "a fairly long string that exceeds any small buffer"};
  for (const char* p : lens) {
    std::string a(p);
    std::string b(a);
    if (b != a || b.size() != a.size() || b.c_str()[b.size()] != '\0') return false;
    if (!a.empty() && b.data() == a.data()) return false;  // distinct storage
    std::string c(std::move(b));
    if (c != a) return false;
    b = "reuse";  // moved-from object is usable
    if (b != "reuse") return false;
    std::string d(a, std::allocator<char>());
    if (d != a) return false;
    std::string e(std::move(d), std::allocator<char>());
    if (e != a) return false;
  }
  return true;
}
static_assert(test_values());

int main() {
  CHECK(test_values());
  {
    using S = std::basic_string<char, std::char_traits<char>, SoccAlloc<char>>;
    S a("xyz", SoccAlloc<char>(1));
    S b(a);
    CHECK(b == a);
    CHECK(b.get_allocator().id == 101);
    S c(std::move(a));
    CHECK(c.get_allocator().id == 1);
    CHECK(c == "xyz");
  }
  {
    using S = std::basic_string<char, std::char_traits<char>, IdAlloc<char>>;
    const char* lit = "a string long enough to need dynamic storage in any implementation";
    S a(lit, IdAlloc<char>(1));
    S b(a, IdAlloc<char>(2));
    CHECK(b == lit);
    CHECK(b.get_allocator().id == 2);
    CHECK(a.get_allocator().id == 1);
    S c(std::move(a), IdAlloc<char>(3));  // unequal allocator: copies the characters
    CHECK(c == lit);
    CHECK(c.get_allocator().id == 3);
    S d(std::move(b), IdAlloc<char>(2));  // equal allocator
    CHECK(d == lit);
    CHECK(d.get_allocator().id == 2);
  }
  return 0;
}
