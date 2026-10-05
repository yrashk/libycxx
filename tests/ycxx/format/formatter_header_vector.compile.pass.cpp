// [vector.syn] declares `template<class T, class charT> requires is-vector-bool-reference<T>
// struct formatter<T, charT>;` ([vector.bool.fmt]), so with only <vector> included:
// formatter<vector<bool, Allocator>::reference, charT> is an enabled specialization
// (semiregular, [formatter.requirements]) for char and wchar_t and any allocator, and
// [format.formatter.spec]/2 provides the formatters of characters, strings, arithmetic types
// and pointers (formatter_spec.hpp); /3: enable_nonlocking_formatter_optimization is true for
// vector<bool>::reference (not specified otherwise).
#include <vector>
#include "formatter_spec.hpp"

template <class T>
struct MyAlloc {
  using value_type = T;
  MyAlloc() = default;
  template <class U>
  MyAlloc(const MyAlloc<U>&) {}
  T* allocate(std::size_t);
  void deallocate(T*, std::size_t);
  friend bool operator==(MyAlloc, MyAlloc) { return true; }
};

using R = std::vector<bool>::reference;
using RA = std::vector<bool, MyAlloc<bool>>::reference;
static_assert(std::semiregular<std::formatter<R, char>>);
static_assert(std::semiregular<std::formatter<R, wchar_t>>);
static_assert(std::semiregular<std::formatter<RA, char>>);
static_assert(std::semiregular<std::formatter<RA, wchar_t>>);
static_assert(std::enable_nonlocking_formatter_optimization<R>);
// parse and format are member templates ([vector.bool.fmt]): nothing to call without <format>.
static_assert(formatter_spec::check());
