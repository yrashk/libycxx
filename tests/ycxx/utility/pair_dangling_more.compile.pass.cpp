// [pairs.pair]/13: pair(U1&&, U2&&) "is defined as deleted if
// reference_constructs_from_temporary_v<first_type, U1&&> is true or
// reference_constructs_from_temporary_v<second_type, U2&&> is true." /17: the constructors
// from pair<U1, U2>&, const pair<U1, U2>&, pair<U1, U2>&&, const pair<U1, U2>&& and pair-like
// P&& are "defined as deleted if reference_constructs_from_temporary_v<first_type,
// decltype(get<0>(FWD(p)))> || reference_constructs_from_temporary_v<second_type,
// decltype(get<1>(FWD(p)))> is true." A deleted constructor is still selected by overload
// resolution, so is_constructible_v is false; when no temporary is involved (the reference
// binds to an lvalue or xvalue of the right type or of a derived class) the constructor works.
#include <utility>
#include <array>
#include <tuple>
#include <type_traits>

struct S {
  S(int);
};
struct D : S {
  D();
};

using CRef = std::pair<const int&, int>;
// const pair&& (get<0> is const long&&) and the other categories
static_assert(!std::is_constructible_v<CRef, const std::pair<long, int>&&>);
static_assert(!std::is_constructible_v<CRef, std::pair<long, int>&>);
static_assert(!std::is_constructible_v<CRef, std::pair<long&, int>&&>);
static_assert(!std::is_constructible_v<CRef, std::pair<const long&, int>&>);
// no temporary: xvalues and lvalues of int itself
static_assert(std::is_constructible_v<CRef, std::pair<int, int>&&>);
static_assert(std::is_constructible_v<CRef, const std::pair<int, int>&&>);
static_assert(std::is_constructible_v<CRef, const std::pair<int, int>&>);
static_assert(std::is_constructible_v<CRef, std::pair<int&, int>>);
static_assert(std::is_constructible_v<CRef, std::pair<int&&, long>>);
// the implicit/explicit choice does not matter: the deleted constructor is not convertible either
static_assert(!std::is_convertible_v<std::pair<long, int>, CRef>);
static_assert(!std::is_convertible_v<const std::pair<long, int>&, CRef>);
static_assert(std::is_convertible_v<std::pair<int, int>&, CRef>);

// the second member
using CRef2 = std::pair<int, const int&>;
static_assert(!std::is_constructible_v<CRef2, int, long>);
static_assert(!std::is_constructible_v<CRef2, int, short&>);
static_assert(!std::is_constructible_v<CRef2, int, double&&>);
static_assert(!std::is_constructible_v<CRef2, std::pair<int, long>>);
static_assert(!std::is_constructible_v<CRef2, const std::pair<int, long>&>);
static_assert(!std::is_constructible_v<CRef2, std::tuple<int, long>>);
static_assert(std::is_constructible_v<CRef2, long, int&>);
static_assert(std::is_constructible_v<CRef2, std::pair<long, int>&>);

// pair-like sources in every value category
static_assert(!std::is_constructible_v<CRef, std::tuple<long, int>>);
static_assert(!std::is_constructible_v<CRef, std::tuple<long, int>&&>);
static_assert(!std::is_constructible_v<CRef, const std::tuple<long, int>&>);
static_assert(!std::is_constructible_v<CRef, const std::tuple<long, int>&&>);
static_assert(std::is_constructible_v<CRef, std::tuple<int, long>&&>);
static_assert(std::is_constructible_v<CRef, const std::tuple<int, long>&>);
using CRefs = std::pair<const int&, const int&>;
static_assert(!std::is_constructible_v<CRefs, std::array<long, 2>&>);
static_assert(!std::is_constructible_v<CRefs, const std::array<short, 2>&>);
static_assert(!std::is_constructible_v<CRefs, std::array<double, 2>&&>);
static_assert(std::is_constructible_v<CRefs, std::array<int, 2>&>);
static_assert(std::is_constructible_v<CRefs, std::array<int, 2>&&>);
static_assert(std::is_constructible_v<CRefs, const std::array<int, 2>&&>);

// class types: a converting constructor creates a temporary; a derived-to-base binding does not
using SRef = std::pair<const S&, int>;
static_assert(!std::is_constructible_v<SRef, int, int>);
static_assert(!std::is_constructible_v<SRef, std::pair<int, int>&>);
static_assert(!std::is_constructible_v<SRef, std::tuple<int, int>&>);
static_assert(std::is_constructible_v<SRef, D&, int>);
static_assert(std::is_constructible_v<SRef, D&&, int>);
static_assert(std::is_constructible_v<SRef, std::pair<D, int>&>);
static_assert(std::is_constructible_v<SRef, std::pair<D, int>&&>);
static_assert(std::is_constructible_v<SRef, std::tuple<D, int>&>);

// rvalue-reference members
using RRef = std::pair<int&&, int>;
static_assert(!std::is_constructible_v<RRef, std::pair<long, int>&&>);
static_assert(!std::is_constructible_v<RRef, std::tuple<long, int>&&>);
static_assert(std::is_constructible_v<RRef, std::pair<int, int>&&>);
static_assert(std::is_constructible_v<RRef, std::tuple<int, int>&&>);
static_assert(!std::is_constructible_v<RRef, std::pair<int, int>&>);  // lvalue cannot bind to int&&

// non-reference members are unaffected
static_assert(std::is_constructible_v<std::pair<int, int>, std::pair<long, double>&>);
static_assert(std::is_constructible_v<std::pair<int, int>, long, double>);
