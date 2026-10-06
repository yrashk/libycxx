// [tuple.elem]/1-6: the four get<I> overloads return T&, T&&, const T& and const T&& for an
// element of non-reference type T, and X& for an element of type X& (Note 1, Note 2: constness is
// shallow; #1 does not turn X& into X&&); an element of type X&& gives X&& from an rvalue tuple
// and X& from an lvalue one. get<T> returns the same, for the type that occurs exactly once,
// including a cv-qualified or reference type (Example 1: get<int> and get<const int> of
// tuple<int, const int, double, double>). All are noexcept and usable in constant expressions.
#include <tuple>
#include <type_traits>
#include <utility>

using std::declval;
using std::get;
using std::is_same_v;
using T = std::tuple<int, long&, char&&, const short>;

static_assert(is_same_v<decltype(get<0>(declval<T&>())), int&>);
static_assert(is_same_v<decltype(get<0>(declval<T&&>())), int&&>);
static_assert(is_same_v<decltype(get<0>(declval<const T&>())), const int&>);
static_assert(is_same_v<decltype(get<0>(declval<const T&&>())), const int&&>);
static_assert(is_same_v<decltype(get<1>(declval<T&>())), long&>);
static_assert(is_same_v<decltype(get<1>(declval<T&&>())), long&>);
static_assert(is_same_v<decltype(get<1>(declval<const T&>())), long&>);
static_assert(is_same_v<decltype(get<1>(declval<const T&&>())), long&>);
static_assert(is_same_v<decltype(get<2>(declval<T&>())), char&>);
static_assert(is_same_v<decltype(get<2>(declval<T&&>())), char&&>);
static_assert(is_same_v<decltype(get<2>(declval<const T&>())), char&>);
static_assert(is_same_v<decltype(get<2>(declval<const T&&>())), char&&>);
static_assert(is_same_v<decltype(get<3>(declval<T&>())), const short&>);
static_assert(is_same_v<decltype(get<3>(declval<T&&>())), const short&&>);

static_assert(is_same_v<decltype(get<int>(declval<T&>())), int&>);
static_assert(is_same_v<decltype(get<int>(declval<const T&&>())), const int&&>);
static_assert(is_same_v<decltype(get<long&>(declval<T&&>())), long&>);
static_assert(is_same_v<decltype(get<char&&>(declval<T&>())), char&>);
static_assert(is_same_v<decltype(get<char&&>(declval<T&&>())), char&&>);
static_assert(is_same_v<decltype(get<const short>(declval<const T&>())), const short&>);

static_assert(noexcept(get<0>(declval<T&>())) && noexcept(get<1>(declval<const T&&>())));
static_assert(noexcept(get<int>(declval<T&&>())) && noexcept(get<const short>(declval<const T&>())));

// Example 1.
constexpr std::tuple<int, const int, double, double> ex(1, 2, 3.4, 5.6);
static_assert(get<int>(ex) == 1 && get<const int>(ex) == 2 && get<2>(ex) == 3.4 && get<3>(ex) == 5.6);

// Constant evaluation, through every overload.
constexpr bool run() {
  long l = 5;
  std::tuple<int, long&> t(1, l);
  get<0>(t) = 10;
  get<long&>(t) = 20;
  const auto& ct = t;
  int moved = get<0>(std::move(t));
  const int&& cr = get<0>(std::move(ct));
  return moved == 10 && cr == 10 && l == 20 && get<1>(ct) == 20 && &get<1>(std::move(ct)) == &l;
}
static_assert(run());
