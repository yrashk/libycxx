// [tuple.cnstr]/15, 23, 27, 31: the converting constructors are "defined as deleted if
// reference_constructs_from_temporary_v<Ti, ...> is true" for some element, so a tuple
// with a reference element cannot be initialized in a way that binds it to a temporary.
// A deleted constructor still takes part in overload resolution, so is_constructible is
// false for those cases.
#include <tuple>
#include <type_traits>
#include <utility>

struct FromInt { FromInt(int); };

// tuple(UTypes&&...)
static_assert(!std::is_constructible_v<std::tuple<const int&>, long>);
static_assert(!std::is_constructible_v<std::tuple<const int&>, long&>);
static_assert(!std::is_constructible_v<std::tuple<const FromInt&>, int>);
static_assert(!std::is_constructible_v<std::tuple<int&&>, long>);
static_assert(!std::is_constructible_v<std::tuple<int, const int&>, int, double>);
static_assert(std::is_constructible_v<std::tuple<const int&>, int&>);   // binds directly
static_assert(std::is_constructible_v<std::tuple<const int&>, int>);    // xvalue binds directly
static_assert(std::is_constructible_v<std::tuple<int, const int&>, long, int&>);
// tuple(const tuple<UTypes...>&) and friends
static_assert(!std::is_constructible_v<std::tuple<const int&>, const std::tuple<long>&>);
static_assert(!std::is_constructible_v<std::tuple<const int&>, std::tuple<long>&>);
static_assert(!std::is_constructible_v<std::tuple<const int&>, std::tuple<long>&&>);
static_assert(!std::is_constructible_v<std::tuple<const int&, int>, std::tuple<long, int>>);
static_assert(std::is_constructible_v<std::tuple<const int&>, std::tuple<int>&>);
static_assert(std::is_constructible_v<std::tuple<const int&>, const std::tuple<int>&>);
// tuple(pair<U1, U2>...)
static_assert(!std::is_constructible_v<std::tuple<const int&, int>, const std::pair<long, int>&>);
static_assert(!std::is_constructible_v<std::tuple<int, const int&>, std::pair<int, long>&&>);
static_assert(!std::is_constructible_v<std::tuple<int, const int&>, std::pair<int, long>&>);
static_assert(std::is_constructible_v<std::tuple<const int&, const int&>, const std::pair<int, int>&>);
// Implicit conversions are also prevented.
static_assert(!std::is_convertible_v<long, std::tuple<const int&>>);
static_assert(!std::is_convertible_v<std::pair<long, int>, std::tuple<const int&, int>>);
