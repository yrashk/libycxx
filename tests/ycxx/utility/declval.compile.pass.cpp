// [declval]: template<class T> add_rvalue_reference_t<T> declval() noexcept; "Remarks: The
// template parameter T of declval may be an incomplete type."
#include <utility>
#include <type_traits>

struct Incomplete;
struct S {
  int f() &;
  long f() &&;
};

static_assert(std::is_same_v<decltype(std::declval<int>()), int&&>);
static_assert(std::is_same_v<decltype(std::declval<int&>()), int&>);
static_assert(std::is_same_v<decltype(std::declval<int&&>()), int&&>);
static_assert(std::is_same_v<decltype(std::declval<const int>()), const int&&>);
static_assert(std::is_same_v<decltype(std::declval<void>()), void>);
static_assert(std::is_same_v<decltype(std::declval<const void>()), void>);  // a prvalue: cv dropped
// returns int(&&)(); a call returning an rvalue reference to function is an lvalue
static_assert(std::is_same_v<decltype(std::declval<int()>()), int (&)()>);
static_assert(std::is_same_v<std::add_rvalue_reference_t<int()>, int (&&)()>);
static_assert(std::is_same_v<decltype(std::declval<Incomplete>()), Incomplete&&>);
static_assert(std::is_same_v<decltype(std::declval<int[]>()), int (&&)[]>);
static_assert(noexcept(std::declval<int>()));
static_assert(std::is_same_v<decltype(std::declval<S>().f()), long>);
static_assert(std::is_same_v<decltype(std::declval<S&>().f()), int>);
// the draft's example
template <class To, class From>
decltype(static_cast<To>(std::declval<From>())) convert(From&&);
static_assert(std::is_same_v<decltype(convert<long>(1)), long>);
