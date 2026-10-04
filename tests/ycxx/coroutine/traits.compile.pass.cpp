// [coroutine.traits.primary]: "if the qualified-id R::promise_type is valid and denotes a type,
// then coroutine_traits<R, ArgTypes...> has the following publicly accessible member: using
// promise_type = R::promise_type; Otherwise, coroutine_traits<R, ArgTypes...> has no members."
#include <coroutine>
#include <type_traits>

struct P {};
struct WithPromise {
  using promise_type = P;
};
struct NoPromise {};
struct NonTypePromise {
  static int promise_type;
};

template <class T>
concept has_promise = requires { typename T::promise_type; };

static_assert(std::is_same_v<std::coroutine_traits<WithPromise>::promise_type, P>);
static_assert(std::is_same_v<std::coroutine_traits<WithPromise, int, double&>::promise_type, P>);
static_assert(!has_promise<std::coroutine_traits<NoPromise>>);
static_assert(!has_promise<std::coroutine_traits<NonTypePromise>>);
static_assert(!has_promise<std::coroutine_traits<int>>);
static_assert(!has_promise<std::coroutine_traits<void>>);
static_assert(std::is_empty_v<std::coroutine_traits<NoPromise>>);
