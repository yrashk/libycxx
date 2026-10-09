// EXPECT-ERROR-GCC: error: conversion from 'MoveOnly' to non-scalar type 'std::any'
// EXPECT-ERROR-CLANG: error: no viable conversion from 'MoveOnly' to 'std::any'
// [any.cons]/6: any(T&&) "Constraints: ... is_copy_constructible_v<VT> is true." A move-only
// value cannot initialise an any.
#include <any>

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
};

std::any a = MoveOnly{};
