// [container.reqmts]/66.4: "No copy constructor or assignment operator of a returned
// iterator throws an exception." Applies to vector ([vector.modifiers] does not override it)
// and, through [string.require]/3 / [container.reqmts], to basic_string; also to the
// const_iterator and reverse iterators returned by cbegin / rbegin. Checked with the
// nothrow traits for both copy and move. Also [forward.iterators]: iterators are default
// constructible.
#include <vector>
#include <string>
#include <type_traits>

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

static_assert(check<std::vector<int>>());
static_assert(check<std::vector<std::string>>());
static_assert(check<std::vector<bool>>());
static_assert(check<std::string>());
static_assert(check<std::wstring>());
