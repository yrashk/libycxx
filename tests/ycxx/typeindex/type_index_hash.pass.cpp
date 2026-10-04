// [type.index.synopsis]: template<> struct hash<type_index>; declared in <typeindex>.
// [type.index]/11: "For an object index of type type_index, hash<type_index>()(index) shall
// evaluate to the same result as index.hash_code()." [unord.hash]/5: an enabled
// specialization is default constructible, copy constructible, copy assignable, swappable, and
// its operator() takes a const type_index& and returns size_t.
#include <typeindex>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

using H = std::hash<std::type_index>;
static_assert(std::is_default_constructible_v<H>);
static_assert(std::is_copy_constructible_v<H>);
static_assert(std::is_copy_assignable_v<H>);
static_assert(std::is_swappable_v<H>);
static_assert(std::is_same_v<std::invoke_result_t<const H&, const std::type_index&>, std::size_t>);
static_assert(std::is_invocable_r_v<std::size_t, const H&, const std::type_info&>);  // via the implicit constructor

struct Local {};

int main() {
  const H h{};
  std::type_index a = typeid(int), b = typeid(Local), c = typeid(const int&);
  CHECK(h(a) == a.hash_code());
  CHECK(h(b) == b.hash_code());
  CHECK(h(a) == h(c));  // equal type_index values hash equally
  CHECK(h(a) == typeid(int).hash_code());
  H copy = h;
  CHECK(copy(b) == h(b));
  return 0;
}
