// [variant.get]: holds_alternative (noexcept), get<I>/get<T> for all four value categories
// with exact return types, bad_variant_access on wrong alternative, get_if<I>/get_if<T>
// (noexcept, nullptr for null pointer or wrong index).
#include <variant>
#include <type_traits>
#include <utility>
#include "check.hpp"

using V = std::variant<int, const long, double>;

static_assert(std::is_same_v<decltype(std::get<0>(std::declval<V&>())), int&>);
static_assert(std::is_same_v<decltype(std::get<0>(std::declval<V&&>())), int&&>);
static_assert(std::is_same_v<decltype(std::get<0>(std::declval<const V&>())), const int&>);
static_assert(std::is_same_v<decltype(std::get<0>(std::declval<const V&&>())), const int&&>);
static_assert(std::is_same_v<decltype(std::get<1>(std::declval<V&>())), const long&>);
static_assert(std::is_same_v<decltype(std::get<1>(std::declval<V&&>())), const long&&>);
static_assert(std::is_same_v<decltype(std::get<double>(std::declval<V&>())), double&>);
static_assert(std::is_same_v<decltype(std::get<double>(std::declval<V&&>())), double&&>);
static_assert(std::is_same_v<decltype(std::get<double>(std::declval<const V&>())), const double&>);
static_assert(std::is_same_v<decltype(std::get<double>(std::declval<const V&&>())), const double&&>);
static_assert(std::is_same_v<decltype(std::get<const long>(std::declval<V&>())), const long&>);
static_assert(std::is_same_v<decltype(std::get_if<0>(std::declval<V*>())), int*>);
static_assert(std::is_same_v<decltype(std::get_if<0>(std::declval<const V*>())), const int*>);
static_assert(std::is_same_v<decltype(std::get_if<1>(std::declval<V*>())), const long*>);
static_assert(std::is_same_v<decltype(std::get_if<double>(std::declval<V*>())), double*>);
static_assert(std::is_same_v<decltype(std::get_if<double>(std::declval<const V*>())), const double*>);
static_assert(noexcept(std::get_if<0>(std::declval<V*>())));
static_assert(noexcept(std::get_if<int>(std::declval<const V*>())));
static_assert(noexcept(std::holds_alternative<int>(std::declval<const V&>())));
static_assert(std::is_same_v<decltype(std::holds_alternative<int>(std::declval<const V&>())), bool>);

constexpr bool test() {
  V v(2.5);
  if (!std::holds_alternative<double>(v) || std::holds_alternative<int>(v)) return false;
  if (std::get<2>(v) != 2.5 || std::get<double>(v) != 2.5) return false;
  std::get<double>(v) = 4.0;
  if (std::get<2>(v) != 4.0) return false;
  if (std::get_if<2>(&v) != &std::get<2>(v)) return false;
  if (std::get_if<0>(&v) != nullptr || std::get_if<int>(&v) != nullptr) return false;
  V* null = nullptr;
  if (std::get_if<0>(null) != nullptr || std::get_if<double>(null) != nullptr) return false;
  const V cv(std::in_place_index<1>, 7L);
  if (std::get<const long>(cv) != 7 || *std::get_if<1>(&cv) != 7) return false;
  double&& rr = std::get<2>(std::move(v));
  if (rr != 4.0) return false;
  // duplicates are accessible by index
  std::variant<int, int> d(std::in_place_index<1>, 3);
  if (std::get<1>(d) != 3 || std::get_if<0>(&d) != nullptr) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  V v(1);
  bool threw = false;
  try { (void)std::get<2>(v); } catch (const std::bad_variant_access& e) { threw = true; (void)e.what(); }
  CHECK(threw);
  threw = false;
  try { (void)std::get<double>(std::move(v)); } catch (const std::exception&) { threw = true; }
  CHECK(threw);
  threw = false;
  const V& cv = v;
  try { (void)std::get<const long>(cv); } catch (const std::bad_variant_access&) { threw = true; }
  CHECK(threw);
  return 0;
}
