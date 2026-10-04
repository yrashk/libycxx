// [meta.rel]/6: is_convertible<From, To> holds iff `To test() { return declval<From>(); }` is
// well-formed, "including any implicit conversions to the return type"; access is checked in an
// unrelated context. Note 4: "well-defined results for reference types, array types, function
// types, and cv void" (so is_convertible<void, void> is true: `return expr-of-type-void;` in a
// function returning void). is_nothrow_convertible: is_convertible_v and the conversion "is known
// not to throw any exceptions".
// [meta.unary.prop]/9: is_constructible<T, Args...> iff `T t(declval<Args>()...);` is
// well-formed (for a function type or cv void T: false). [dcl.init.general]/16.6.2.2 (P0960):
// parenthesized aggregate initialization applies to aggregates and arrays.
// /11: is_assignable<T, U> iff declval<T>() = declval<U>() is well-formed as an unevaluated
// operand.
#include <type_traits>

struct Base {};
struct Derived : Base {};
struct PrivDerived : private Base {};
struct Amb1 : Base {};
struct Amb2 : Base {};
struct Amb : Amb1, Amb2 {};
struct Abstract { virtual void f() = 0; };
struct Explicit { explicit Explicit(int); };
struct Implicit { Implicit(int); };
struct NothrowImplicit { NothrowImplicit(int) noexcept; };
struct ConvOp { operator int() const; };
struct NothrowConvOp { operator int() const noexcept; };
struct PrivConv { private: operator int() const; };
struct ExplicitOp { explicit operator int() const; };
struct NoMove { NoMove(); NoMove(NoMove&&) = delete; };
struct Agg { int a; long b; };
struct DelAssign { DelAssign& operator=(const DelAssign&) = delete; };
struct RvalueAssign { RvalueAssign& operator=(int) &&; };
struct LvalueAssign { LvalueAssign& operator=(int) &; };

template <class F, class T> constexpr bool cv = std::is_convertible_v<F, T> && std::is_convertible<F, T>::value &&
    std::is_base_of_v<std::true_type, std::is_convertible<F, T>>;
template <class F, class T> constexpr bool ncv = !std::is_convertible_v<F, T> && !std::is_nothrow_convertible_v<F, T> &&
    std::is_base_of_v<std::false_type, std::is_convertible<F, T>> &&
    std::is_base_of_v<std::false_type, std::is_nothrow_convertible<F, T>>;

// void and functions/arrays
static_assert(cv<void, void> && cv<const void, void> && cv<void, const volatile void>);
static_assert(std::is_nothrow_convertible_v<void, void>);
static_assert(ncv<void, int> && ncv<int, void> && ncv<int*, void>);
static_assert(ncv<void(), void()> && ncv<int, int()> && ncv<int(), int>);
static_assert(cv<void(), void (*)()> && cv<void(), void (&)()> && cv<void(), void (&&)()>);
static_assert(cv<void() noexcept, void (*)()> && ncv<void(), void (*)() noexcept>);
static_assert(cv<void (&)(), void (*)()> && cv<void (&)() noexcept, void (&)()>);
static_assert(ncv<void() const, void (*)()> && ncv<void() const, void() const>);
static_assert(ncv<int[3], int[3]> && ncv<int*, int[3]> && ncv<int[], int[]>);
static_assert(cv<int[3], int*> && cv<int[], int*> && cv<int[3], const int*> && cv<int (&)[3], int*>);
static_assert(cv<int[3], int (&&)[3]> && cv<int[3], const int (&)[3]> && ncv<int[3], int (&)[3]>);
static_assert(cv<int (&)[3], int (&)[3]> && ncv<int (&)[3], int (&)[4]> && ncv<int (&)[3], long (&)[3]>);
// references and temporaries
static_assert(cv<int, const int&> && ncv<int, int&> && cv<int, int&&> && cv<long, int&&> && cv<long, const int&>);
static_assert(cv<int&, int&> && ncv<int&, int&&> && ncv<const int&, int&> && cv<int&&, const int&>);
static_assert(cv<int&, volatile const int&>);   // binding an lvalue: fine
static_assert(ncv<int, const volatile int&>);              // rvalue to const volatile & is ill-formed
static_assert(cv<Derived&, Base&> && cv<Derived, Base> && cv<Derived*, Base*> && cv<Derived&&, Base&&>);
static_assert(ncv<Base&, Derived&> && ncv<Base*, Derived*> && ncv<Base, Derived>);
static_assert(ncv<PrivDerived&, Base&> && ncv<PrivDerived*, Base*> && ncv<Amb*, Base*> && ncv<Amb, Base>);
static_assert(cv<int Base::*, int Derived::*> && ncv<int Derived::*, int Base::*>);
static_assert(cv<std::nullptr_t, int*> && cv<std::nullptr_t, int Base::*> && ncv<int*, std::nullptr_t>);
static_assert(ncv<std::nullptr_t, bool>);   // copy-initialization of bool from nullptr_t is ill-formed
static_assert(cv<int*, bool> && cv<int Base::*, bool> && cv<double, int> && cv<int, double>);
static_assert(ncv<int*, void*&> && cv<int*, void*> && cv<int*, const void*> && ncv<const int*, void*>);
static_assert(ncv<int**, const int**> && cv<int**, const int* const*>);   // [conv.qual]
// class types
static_assert(ncv<Abstract, Abstract> && cv<Abstract&, Abstract&> && ncv<int, Abstract>);
static_assert(ncv<int, Explicit> && cv<int, Implicit> && cv<ConvOp, int> && cv<ConvOp, long>);
static_assert(ncv<PrivConv, int> && ncv<ExplicitOp, int> && ncv<ExplicitOp, bool>);
static_assert(ncv<NoMove, NoMove> && cv<NoMove&, NoMove&> && ncv<NoMove&, NoMove>);
static_assert(ncv<Agg, int>);
static_assert(cv<Agg, Agg> && cv<const Agg&, Agg>);
// is_nothrow_convertible
static_assert(std::is_nothrow_convertible_v<int, long> && std::is_nothrow_convertible_v<Derived*, Base*>);
static_assert(std::is_nothrow_convertible_v<int, NothrowImplicit> && !std::is_nothrow_convertible_v<int, Implicit>);
static_assert(std::is_nothrow_convertible_v<NothrowConvOp, int> && !std::is_nothrow_convertible_v<ConvOp, int>);
static_assert(std::is_nothrow_convertible_v<int, const long&> && std::is_nothrow_convertible_v<Derived&, Base&>);
static_assert(std::is_nothrow_convertible_v<NothrowConvOp, const int&> && !std::is_nothrow_convertible_v<ConvOp, const int&>);
static_assert(std::is_nothrow_convertible_v<void, void>);
static_assert(std::is_nothrow_convertible_v<void(), void (*)()>);

// is_constructible
template <class T, class... A> constexpr bool ct = std::is_constructible_v<T, A...> && std::is_base_of_v<std::true_type, std::is_constructible<T, A...>>;
template <class T, class... A> constexpr bool nct = !std::is_constructible_v<T, A...> && !std::is_nothrow_constructible_v<T, A...> &&
    !std::is_trivially_constructible_v<T, A...> && std::is_base_of_v<std::false_type, std::is_constructible<T, A...>>;
static_assert(nct<void> && nct<const void> && nct<void, void> && nct<void()> && nct<void(), void()> && nct<void() const>);
static_assert(nct<int[]> && nct<int&> && nct<int&&> && nct<Abstract>);
static_assert(ct<int> && ct<int[3]> && ct<int[2][2]> && ct<int*> && ct<Agg> && ct<int&, int&> && ct<const int&, int>);
static_assert(nct<int&, long&> && ct<const int&, long&> && ct<int&&, long&> && nct<int&&, int&>);
static_assert(ct<Derived&, Derived&> && ct<Base&, Derived&> && nct<Derived&, Base&>);
static_assert(nct<Derived&, Base&>);   // no implicit downcast
static_assert(ct<int, ConvOp> && ct<int, ExplicitOp>);   // direct-initialization considers explicit operator int
static_assert(nct<bool, ExplicitOp>);   // [over.match.conv]/1.2: int to bool is not a qualification conversion
static_assert(ct<Explicit, int> && ct<Implicit, int> && nct<Explicit, int*>);
static_assert(nct<int&, ConvOp>);
static_assert(ct<void (&)(), void()> && ct<void (*)(), void()> && ct<void (*)(), void (&)()>);
static_assert(ct<int, int> && ct<int, double> && nct<int, int, int> && nct<int*, int>);
static_assert(ct<Agg, int> && ct<Agg, int, long> && ct<Agg, int, int> && nct<Agg, int, long, int>);   // P0960
static_assert(ct<int[3], int> && ct<int[3], int, int, int> && nct<int[3], int, int, int, int>);         // P0960
static_assert(nct<Agg, int*>);
static_assert(ct<NoMove> && nct<NoMove, NoMove> && nct<NoMove, NoMove&>);
static_assert(std::is_trivially_constructible_v<int, int&> && std::is_trivially_constructible_v<int, long>);
static_assert(std::is_trivially_constructible_v<Agg, const Agg&> && !std::is_trivially_constructible_v<Implicit, int>);
static_assert(std::is_nothrow_constructible_v<NothrowImplicit, int> && !std::is_nothrow_constructible_v<Implicit, int>);
static_assert(std::is_nothrow_constructible_v<int, NothrowConvOp> && !std::is_nothrow_constructible_v<int, ConvOp>);
static_assert(std::is_default_constructible_v<int[3]> && !std::is_default_constructible_v<int[]> && !std::is_default_constructible_v<int&>);
static_assert(!std::is_copy_constructible_v<void() const> && !std::is_move_constructible_v<void() &>);
static_assert(std::is_copy_constructible_v<int&> && std::is_move_constructible_v<int&&> && !std::is_copy_constructible_v<int[3]>);
static_assert(!std::is_copy_constructible_v<void> && !std::is_move_constructible_v<const void>);

// is_assignable
template <class T, class U> constexpr bool as = std::is_assignable_v<T, U> && std::is_base_of_v<std::true_type, std::is_assignable<T, U>>;
template <class T, class U> constexpr bool nas = !std::is_assignable_v<T, U> && !std::is_nothrow_assignable_v<T, U> &&
    !std::is_trivially_assignable_v<T, U> && std::is_base_of_v<std::false_type, std::is_assignable<T, U>>;
static_assert(as<int&, int> && as<int&, double> && as<int&, int&> && nas<int, int> && nas<int&&, int> && nas<const int&, int>);
static_assert(nas<void, void> && nas<void, int> && nas<int&, void> && nas<int (&)[3], int (&)[3]> && nas<int[3], int[3]>);
static_assert(nas<void (&)(), void (&)()> && nas<void(), void()>);
static_assert(as<int*&, std::nullptr_t> && nas<int*&, int> && as<int*&, int[3]> && as<bool&, int*>);
static_assert(as<Base&, Derived> && as<Base, Base> && as<Base&&, Base>);   // class rvalues are assignable
static_assert(nas<DelAssign&, const DelAssign&> && nas<DelAssign&, DelAssign>);
static_assert(as<RvalueAssign, int> && as<RvalueAssign&&, int> && nas<RvalueAssign&, int>);
static_assert(as<LvalueAssign&, int> && nas<LvalueAssign, int> && nas<LvalueAssign&&, int>);
static_assert(std::is_copy_assignable_v<int> && !std::is_copy_assignable_v<const int> && !std::is_copy_assignable_v<int[2]>);
static_assert(std::is_move_assignable_v<int&> && !std::is_move_assignable_v<void> && !std::is_copy_assignable_v<void() const>);
static_assert(std::is_trivially_assignable_v<int&, int> && std::is_trivially_assignable_v<int&, double>);
static_assert(std::is_nothrow_assignable_v<int&, int> && std::is_nothrow_copy_assignable_v<Agg>);

int main() {}
