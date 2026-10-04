// [set.overview], [multiset.overview], [associative.reqmts.general]/6, /8-17: key_type and
// value_type are Key, value_compare is key_compare, iterator and const_iterator are both
// constant bidirectional iterators, node_type is a node handle with value_type and
// allocator_type (no key_type / mapped_type), insert_return_type exists for set only;
// iterator copy and move do not throw ([container.reqmts]/66.4).
#include <set>
#include <functional>
#include <type_traits>
#include "container_values.hpp"
#include "reqs/associative.hpp"
#include "reqs/container_types.hpp"
#include "reqs/iterator_nothrow.hpp"

static_assert(std::is_same_v<std::set<int>::key_compare, std::less<int>>);
static_assert(std::is_same_v<std::set<int>::value_compare, std::less<int>>);
static_assert(std::is_same_v<std::multiset<int, std::greater<>>::value_compare, std::greater<>>);
static_assert(std::is_same_v<std::set<int>::allocator_type, std::allocator<int>>);
static_assert(std::is_same_v<std::set<Elem>::const_pointer, const Elem*>);
static_assert(reqs::associative::types<std::set<int>>());
static_assert(reqs::associative::types<std::set<Elem>>());
static_assert(reqs::associative::types<std::multiset<int>>());
static_assert(reqs::associative::types<std::multiset<Elem, std::greater<>>>());
static_assert(reqs::iterator_nothrow::check<std::set<int>>() && reqs::iterator_nothrow::check<std::multiset<Elem>>());
static_assert(std::is_same_v<std::set<int>::node_type, std::multiset<int, std::greater<int>>::node_type>);

int main() { return 0; }
