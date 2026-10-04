// [meta.unary.prop] reference_constructs_from_temporary<T, U>: "T is a reference type, and
// the initialization T t(VAL<U>); is well-formed and binds t to a temporary object whose
// lifetime is extended". reference_converts_from_temporary: same with T t = VAL<U>;
// (copy-initialization). VAL<U> is an rvalue of type U for non-reference U, otherwise
// declval<U>() ([meta.unary.prop]).
#include <type_traits>

struct ToInt { operator int() const; };                 // yields a prvalue int
struct ToIntRef { operator int&() const; };             // yields an lvalue: binds directly
struct Base {};
struct Derived : Base {};

template <class T, class U>
constexpr bool both = std::reference_constructs_from_temporary_v<T, U> &&
                      std::reference_converts_from_temporary_v<T, U>;
template <class T, class U>
constexpr bool neither = !std::reference_constructs_from_temporary_v<T, U> &&
                         !std::reference_converts_from_temporary_v<T, U>;

static_assert(std::is_base_of_v<std::true_type, std::reference_constructs_from_temporary<const int&, int>>);
static_assert(std::is_base_of_v<std::false_type, std::reference_converts_from_temporary<int&, int&>>);

// Prvalue (VAL<int> is a prvalue) materialized into a temporary.
static_assert(both<const int&, int>);
static_assert(both<int&&, int>);
static_assert(both<const int&&, int>);
// Conversion creates a temporary.
static_assert(both<const int&, long>);
static_assert(both<const int&, long&>);
static_assert(both<int&&, long&>);
static_assert(both<const int&, double&&>);
static_assert(both<const long&, const int&>);
static_assert(both<const int&, ToInt>);
static_assert(both<const Base&, Derived>);              // prvalue Derived -> temporary
// Direct bindings: no temporary.
static_assert(neither<const int&, int&>);
static_assert(neither<const int&, const int&>);
static_assert(neither<const int&, int&&>);              // xvalue binds directly
static_assert(neither<int&&, int&&>);
static_assert(neither<const int&, ToIntRef>);
static_assert(neither<const Base&, Derived&>);
static_assert(neither<const Base&, Derived&&>);
static_assert(neither<Base&&, Derived&&>);
// Ill-formed initializations.
static_assert(neither<int&, int>);
static_assert(neither<int&, long&>);
static_assert(neither<int&, const int&>);
static_assert(neither<int&&, int&>);
static_assert(neither<const int&, void>);
static_assert(neither<const int&, int*>);
// Non-reference T is always false.
static_assert(neither<int, int>);
static_assert(neither<int, long>);
static_assert(neither<Base, Derived>);
static_assert(neither<void, void>);
