// basic_string with an allocator whose allocate throws, during constant evaluation (P3068;
// basic_string and allocator use are constexpr): [string.require]/2: "If any member function or
// operator of basic_string throws an exception, that function or operator has no other effect on
// the basic_string object"; [string.require]/3, [container.alloc.reqmts]: storage comes from the
// allocator, whose exception propagates ([allocator.requirements.general]: allocate may throw).
// After each failed member, the contents, size and capacity are unchanged and the string is
// still usable.
// XFAIL: clang Clang 23 cannot throw during constant evaluation (P3068's core-language part)
// REQUIRES: exceptions
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include "check.hpp"

template <class T>
struct budget_alloc {
  using value_type = T;
  int* budget;
  constexpr explicit budget_alloc(int* b) : budget(b) {}
  template <class U>
  constexpr budget_alloc(const budget_alloc<U>& o) : budget(o.budget) {}
  constexpr T* allocate(std::size_t n) {
    if (*budget == 0) throw std::bad_alloc();
    --*budget;
    return std::allocator<T>().allocate(n);
  }
  constexpr void deallocate(T* p, std::size_t n) { std::allocator<T>().deallocate(p, n); }
  template <class U>
  constexpr bool operator==(const budget_alloc<U>& o) const { return budget == o.budget; }
};

using str = std::basic_string<char, std::char_traits<char>, budget_alloc<char>>;

template <class F>
constexpr bool bad_alloc(F f) {
  try {
    f();
  } catch (const std::bad_alloc&) {
    return true;
  } catch (...) {
    return false;
  }
  return false;
}

constexpr bool members_have_no_effect() {
  int budget = 1;
  str s("a string long enough to need allocated storage", budget_alloc<char>(&budget));
  const std::string_view orig = "a string long enough to need allocated storage";
  const auto cap = s.capacity();
  const char* data = s.data();
  const std::string_view more = "and more text, so that the capacity must grow beyond what it is";
  int ok = 0;
  ok += bad_alloc([&] { s.append(more); });
  ok += bad_alloc([&] { s += more; });
  ok += bad_alloc([&] { s.insert(0, more); });
  ok += bad_alloc([&] { s.replace(0, 1, more); });
  ok += bad_alloc([&] { s.resize(cap + 1, 'x'); });
  ok += bad_alloc([&] { s.reserve(cap + 1); });
  ok += bad_alloc([&] { s.assign(cap + 1, 'y'); });
  ok += bad_alloc([&] { str copy(s); });
  ok += bad_alloc([&] { (void)(s + more); });
  // no effects: the same characters in the same storage
  ok += std::string_view(s) == orig && s.capacity() == cap && s.data() == data;
  // still usable: an operation within the capacity needs no allocation
  s.erase(1);
  s.push_back('!');
  ok += std::string_view(s) == "a!";
  return ok == 11;
}
static_assert(members_have_no_effect());

int main() { CHECK(members_have_no_effect()); }
