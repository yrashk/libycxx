// [support.initlist], [support.initlist.access], [support.initlist.cons]:
// initializer_list<E>::data() returns begin(); empty() returns size() == 0; a
// default-constructed list has size() == 0; begin()/end() of an empty list are identical;
// all members are constexpr and noexcept, and the member types are as in the synopsis.
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include "check.hpp"

using IL = std::initializer_list<int>;
static_assert(std::is_same_v<IL::value_type, int>);
static_assert(std::is_same_v<IL::reference, const int&>);
static_assert(std::is_same_v<IL::const_reference, const int&>);
static_assert(std::is_same_v<IL::size_type, std::size_t>);
static_assert(std::is_same_v<IL::iterator, const int*>);
static_assert(std::is_same_v<IL::const_iterator, const int*>);
static_assert(std::is_same_v<decltype(std::declval<const IL&>().data()), const int*>);
static_assert(std::is_same_v<decltype(std::declval<const IL&>().empty()), bool>);
static_assert(std::is_same_v<decltype(std::declval<const IL&>().size()), std::size_t>);
static_assert(noexcept(std::declval<const IL&>().data()));
static_assert(noexcept(std::declval<const IL&>().empty()));
static_assert(noexcept(IL()));
static_assert(std::is_trivially_copyable_v<IL>);

constexpr bool test() {
  IL il = {3, 1, 4, 1, 5};
  if (il.data() != il.begin()) return false;
  if (il.empty() || il.size() != 5) return false;
  if (il.data()[2] != 4 || il.end() != il.begin() + il.size()) return false;
  IL copy = il;                                   // copies do not copy the elements
  if (copy.data() != il.data()) return false;
  IL e;
  if (!e.empty() || e.size() != 0 || e.begin() != e.end() || e.data() != e.begin()) return false;
  IL e2 = {};
  if (!e2.empty() || e2.begin() != e2.end()) return false;
  std::initializer_list<const char*> one = {"x"};
  if (one.empty() || one.size() != 1 || one.data()[0][0] != 'x') return false;
  return true;
}
static_assert(test());
static_assert(std::initializer_list<int>{}.empty());
static_assert(std::initializer_list<double>{1.0, 2.0}.data()[1] == 2.0);

int main() {
  CHECK(test());
  return 0;
}
