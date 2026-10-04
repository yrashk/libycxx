// [container.reqmts]/2-9: X::value_type is T; X::reference is T&; X::const_reference is const
// T&; X::iterator meets the forward iterator requirements with value type T and is convertible
// to X::const_iterator; X::const_iterator is a constant forward iterator with value type T;
// X::difference_type is a signed integer type identical to the difference type of iterator
// and const_iterator; X::size_type is an unsigned integer type that can represent any
// non-negative value of difference_type. [forward.iterators]/1: the reference type of a
// mutable forward iterator is T&, of a constant one const T&. [container.rev.reqmts]/2-3:
// reverse_iterator is reverse_iterator<iterator>, const_reverse_iterator is
// reverse_iterator<const_iterator>. [container.alloc.reqmts]/4-5: allocator_type, whose
// value_type is X::value_type.
// Written as a template to be instantiated for every container (vector and basic_string now,
// [basic.string.general]/2: basic_string is a contiguous container). vector<bool> is not a
// container in this sense (its reference is a proxy, [vector.bool.pspc]).
#include <vector>
#include <string>
#include <concepts>
#include <iterator>
#include <limits>
#include <type_traits>
#include "container_values.hpp"

#include "reqs/container_types.hpp"

using namespace reqs::container_types;

struct Pod {
  int a;
  double b;
};

static_assert(container_types<std::vector<int>, int>());
static_assert(container_types<std::vector<const int*>, const int*>());
static_assert(container_types<std::vector<Pod>, Pod>());
static_assert(container_types<std::vector<Elem>, Elem>());
static_assert(container_types<std::vector<std::string>, std::string>());
static_assert(container_types<std::vector<std::vector<int>>, std::vector<int>>());
static_assert(container_types<std::string, char>());
static_assert(container_types<std::wstring, wchar_t>());
static_assert(container_types<std::u8string, char8_t>());
static_assert(container_types<std::u16string, char16_t>());
static_assert(container_types<std::u32string, char32_t>());
