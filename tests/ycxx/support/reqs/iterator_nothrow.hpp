// Generic requirement checks extracted from tests/ycxx/containers/iterator_nothrow.compile.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <type_traits>

namespace reqs::iterator_nothrow {

template <class It>
constexpr bool nothrow_copy() {
  static_assert(std::is_nothrow_copy_constructible_v<It>);
  static_assert(std::is_nothrow_copy_assignable_v<It>);
  static_assert(std::is_nothrow_move_constructible_v<It>);
  static_assert(std::is_nothrow_move_assignable_v<It>);
  static_assert(std::is_default_constructible_v<It>);
  return true;
}

template <class X>
constexpr bool check() {
  return nothrow_copy<typename X::iterator>() && nothrow_copy<typename X::const_iterator>() &&
         nothrow_copy<typename X::reverse_iterator>() && nothrow_copy<typename X::const_reverse_iterator>();
}

}  // namespace reqs::iterator_nothrow
