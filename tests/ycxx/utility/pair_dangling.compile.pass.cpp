// [pairs.pair]/13: pair(U1&&, U2&&) "is defined as deleted if
// reference_constructs_from_temporary_v<first_type, U1&&> is true or
// reference_constructs_from_temporary_v<second_type, U2&&> is true." /17: the converting
// constructors from pair<U1, U2> / pair-like P are likewise deleted when a reference member
// would bind to a temporary created from get<i>(FWD(p)). A deleted constructor still takes part
// in overload resolution, so is_constructible is false.
#include <utility>
#include <tuple>
#include <type_traits>

using CRef = std::pair<const int&, int>;
static_assert(std::is_constructible_v<CRef, const int&, int>);
static_assert(std::is_constructible_v<CRef, int&, int>);
static_assert(std::is_constructible_v<CRef, int&&, int>);  // binds to the xvalue, no temporary
static_assert(!std::is_constructible_v<CRef, long, int>);   // would bind to a temporary int
static_assert(!std::is_constructible_v<CRef, long&, int>);
static_assert(!std::is_constructible_v<CRef, double, int>);
static_assert(!std::is_constructible_v<std::pair<int, const long&>, int, int>);
static_assert(!std::is_constructible_v<std::pair<int, const long&>, int, const int&>);
// from pairs
static_assert(std::is_constructible_v<CRef, std::pair<int, int>&>);
static_assert(!std::is_constructible_v<CRef, std::pair<long, int>&>);
static_assert(!std::is_constructible_v<CRef, const std::pair<long, int>&>);
static_assert(!std::is_constructible_v<CRef, std::pair<long, int>&&>);
// from pair-like
static_assert(std::is_constructible_v<CRef, std::tuple<int, int>&>);
static_assert(!std::is_constructible_v<CRef, std::tuple<short, int>&>);
// rvalue reference members
using RRef = std::pair<int&&, int>;
static_assert(std::is_constructible_v<RRef, int, int>);   // binds to the argument xvalue
static_assert(!std::is_constructible_v<RRef, long, int>);
