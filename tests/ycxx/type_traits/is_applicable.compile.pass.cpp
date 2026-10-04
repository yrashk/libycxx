// [meta.rel] is_applicable / is_nothrow_applicable and [meta.trans.other] apply_result:
// "tuple-like<Tuple> is true and the expression INVOKE(declval<Fn>(), ELEMS-OF(Tuple)...)
// is well-formed", where ELEMS-OF(T) is get<N>(declval<T>())... ([meta.rel]/3).
// [tuple.like]: tuple-like is satisfied only by specializations of array, complex, pair,
// tuple and ranges::subrange.
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

struct Overloaded {
  char operator()(int&) const;
  short operator()(const int&) const;
  long operator()(int&&) const;
  long long operator()(const int&&) const;
};
struct NoThrow {
  int operator()(int, double) const noexcept;
};
struct MayThrow {
  int operator()(int, double) const;
};
struct MixedNoexcept {
  void operator()(int&) const noexcept;
  void operator()(int&&) const;
};
struct S {
  int data;
  int fn(int) const;
  int nt(int) const noexcept;
};
struct Nullary {
  double operator()() const noexcept;
};

// A user type with a tuple protocol is NOT tuple-like ([tuple.like]).
struct MyTuple { int a, b; };
template <> struct std::tuple_size<MyTuple> : std::integral_constant<std::size_t, 2> {};
template <std::size_t I> struct std::tuple_element<I, MyTuple> { using type = int; };
template <std::size_t I> int& get(MyTuple&);

using Add = int (*)(int, int);

// Base characteristics.
static_assert(std::is_base_of_v<std::true_type, std::is_applicable<Add, std::tuple<int, int>>>);
static_assert(std::is_base_of_v<std::false_type, std::is_applicable<Add, std::tuple<int>>>);
static_assert(std::is_base_of_v<std::true_type, std::is_nothrow_applicable<NoThrow, std::tuple<int, double>>>);
static_assert(std::is_base_of_v<std::false_type, std::is_nothrow_applicable<MayThrow, std::tuple<int, double>>>);

// Basic applicability with tuple and pair.
static_assert(std::is_applicable_v<Add, std::tuple<int, int>>);
static_assert(std::is_applicable_v<Add, std::tuple<short, long>&>);
static_assert(std::is_applicable_v<Add, std::pair<int, int>>);
static_assert(std::is_applicable_v<Add, const std::pair<char, int>&>);
static_assert(!std::is_applicable_v<Add, std::tuple<int>>);
static_assert(!std::is_applicable_v<Add, std::tuple<int, int, int>>);
static_assert(!std::is_applicable_v<Add, std::tuple<int, int*>>);
static_assert(std::is_applicable_v<Nullary, std::tuple<>>);
static_assert(std::is_applicable_v<Nullary, const std::tuple<>&>);
static_assert(!std::is_applicable_v<Nullary, std::tuple<int>>);

// Not tuple-like: false, and apply_result has no member type.
static_assert(!std::is_applicable_v<Add, MyTuple>);
static_assert(!std::is_applicable_v<Add, MyTuple&>);
static_assert(!std::is_applicable_v<Nullary, int>);
static_assert(!std::is_applicable_v<Add, int[2]>);
static_assert(!std::is_nothrow_applicable_v<Nullary, int>);
template <class F, class T> concept HasApplyResult = requires { typename std::apply_result<F, T>::type; };
static_assert(!HasApplyResult<Add, MyTuple>);
static_assert(!HasApplyResult<Add, std::tuple<int>>);
static_assert(!HasApplyResult<Add, void>);
static_assert(HasApplyResult<Add, std::tuple<int, int>>);

// Value category of the elements follows the value category of Tuple.
static_assert(std::is_same_v<std::apply_result_t<Overloaded, std::tuple<int>>, long>);
static_assert(std::is_same_v<std::apply_result_t<Overloaded, std::tuple<int>&>, char>);
static_assert(std::is_same_v<std::apply_result_t<Overloaded, const std::tuple<int>&>, short>);
static_assert(std::is_same_v<std::apply_result_t<Overloaded, const std::tuple<int>>, long long>);
static_assert(std::is_same_v<std::apply_result_t<Overloaded, std::tuple<int>&&>, long>);
static_assert(std::is_same_v<std::apply_result<Overloaded, std::tuple<int&>>::type, char>);       // get of int& yields int&
static_assert(std::is_same_v<std::apply_result_t<Overloaded, std::tuple<int&&>&>, char>);          // named rvalue-ref member is an lvalue
static_assert(std::is_same_v<std::apply_result_t<Overloaded, std::tuple<int&&>>, long>);
static_assert(std::is_same_v<std::apply_result_t<Nullary, std::tuple<>>, double>);
static_assert(std::is_applicable_v<void (*)(int&), std::tuple<int>&>);
static_assert(!std::is_applicable_v<void (*)(int&), std::tuple<int>>);
static_assert(!std::is_applicable_v<void (*)(int&), const std::tuple<int>&>);
static_assert(std::is_applicable_v<void (*)(int&&), std::tuple<int>>);
static_assert(!std::is_applicable_v<void (*)(int&&), std::tuple<int>&>);

// INVOKE semantics: pointers to members.
static_assert(std::is_applicable_v<int S::*, std::tuple<S&>>);
static_assert(std::is_same_v<std::apply_result_t<int S::*, std::tuple<S&>>, int&>);
static_assert(std::is_same_v<std::apply_result_t<int S::*, std::tuple<S>>, int&&>);
static_assert(std::is_same_v<std::apply_result_t<int S::*, std::tuple<const S*>>, const int&>);
static_assert(std::is_same_v<std::apply_result_t<int (S::*)(int) const, std::tuple<S*, int>>, int>);
static_assert(std::is_same_v<std::apply_result_t<int (S::*)(int) const, std::pair<const S&, long>>, int>);
static_assert(!std::is_applicable_v<int (S::*)(int) const, std::tuple<int, S*>>);

// nothrow.
static_assert(std::is_nothrow_applicable_v<NoThrow, std::tuple<int, double>>);
static_assert(std::is_nothrow_applicable_v<NoThrow, std::pair<int, double>&>);
static_assert(!std::is_nothrow_applicable_v<MayThrow, std::tuple<int, double>>);
static_assert(std::is_applicable_v<MayThrow, std::tuple<int, double>>);
static_assert(std::is_nothrow_applicable_v<MixedNoexcept, std::tuple<int>&>);
static_assert(!std::is_nothrow_applicable_v<MixedNoexcept, std::tuple<int>>);
static_assert(!std::is_nothrow_applicable_v<NoThrow, std::tuple<int>>);           // not applicable at all
static_assert(std::is_nothrow_applicable_v<int (S::*)(int) const noexcept, std::tuple<S&, int>>);
static_assert(!std::is_nothrow_applicable_v<int (S::*)(int) const, std::tuple<S&, int>>);
static_assert(std::is_nothrow_applicable_v<Nullary, std::tuple<>>);
// A conversion that may throw in the argument makes the expression potentially-throwing.
struct ThrowingConv { operator int() const; };
static_assert(!std::is_nothrow_applicable_v<NoThrow, std::tuple<ThrowingConv, double>>);
static_assert(std::is_applicable_v<NoThrow, std::tuple<ThrowingConv, double>>);
