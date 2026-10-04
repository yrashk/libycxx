// [utility.as.const]: template<class T> constexpr add_const_t<T>& as_const(T& t) noexcept;
// "Returns: t." [utility.syn]: template<class T> void as_const(const T&&) = delete;
#include <utility>
#include <type_traits>

int i = 0;
const int ci = 0;
static_assert(std::is_same_v<decltype(std::as_const(i)), const int&>);
static_assert(std::is_same_v<decltype(std::as_const(ci)), const int&>);
static_assert(noexcept(std::as_const(i)));
int arr[2];
static_assert(std::is_same_v<decltype(std::as_const(arr)), const int (&)[2]>);

template <class T>
concept as_const_ok = requires(T&& t) { std::as_const(std::forward<T>(t)); };
static_assert(as_const_ok<int&>);
static_assert(as_const_ok<const int&>);
static_assert(!as_const_ok<int>);  // rvalues select the deleted overload
static_assert(!as_const_ok<const int>);

constexpr bool test() {
  int x = 3;
  const int& r = std::as_const(x);
  return &r == &x && r == 3;
}
static_assert(test());
