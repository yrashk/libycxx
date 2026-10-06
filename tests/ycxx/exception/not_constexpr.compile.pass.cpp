// [constexpr.functions]/1: an implementation shall not declare a standard library function
// constexpr unless the document requires it. [exception.syn] (after P3818, which kept P3068's
// constexpr exceptions but removed constexpr from the functions whose results depend on the
// evaluation's context): current_exception() and uncaught_exceptions() are not constexpr (the
// constexpr current-exception is exposition only), and neither are nested_exception's members
// ([except.nested]: its default constructor calls current_exception()), throw_with_nested and
// rethrow_if_nested. So none of them can be evaluated in a constant expression; in particular a
// const local initialized from uncaught_exceptions() or current_exception() is never constant
// initialized and evaluates at run time.
#include <exception>
#include <type_traits>

// Each concept is satisfied iff the expression, made dependent on T, is a constant expression.
template <class T>
concept uncaught_is_constant =
    requires { typename std::integral_constant<int, std::uncaught_exceptions() + sizeof(T) * 0>; };
template <class T>
concept current_is_constant =
    requires { typename std::bool_constant<(std::current_exception() == nullptr) && sizeof(T) != 0>; };

struct Poly {
  virtual ~Poly() = default;
};
constexpr bool call_rethrow_if_nested(const Poly& p) {
  std::rethrow_if_nested(p);
  return true;
}
template <class T>
concept rethrow_if_nested_is_constant =
    requires { typename std::bool_constant<call_rethrow_if_nested(Poly{}) && sizeof(T) != 0>; };

static_assert(!uncaught_is_constant<int>);
static_assert(!current_is_constant<int>);
static_assert(!rethrow_if_nested_is_constant<int>);

constexpr bool make_nested() {
  std::nested_exception e;
  return e.nested_ptr() == nullptr;
}
template <class T>
concept nested_is_constant = requires { typename std::bool_constant<make_nested() && sizeof(T) != 0>; };
static_assert(!nested_is_constant<int>);

int main() {
  // At run time they work, outside any handler.
  const int n = std::uncaught_exceptions();
  const bool none = std::current_exception() == nullptr;
  return (n == 0 && none) ? 0 : 1;
}
