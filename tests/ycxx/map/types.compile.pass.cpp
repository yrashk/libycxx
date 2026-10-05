// [map.overview], [multimap.overview], [associative.reqmts.general]/6, /8-17: key_type,
// mapped_type, value_type = pair<const Key, T>, key_compare, value_compare (an ordering on
// pairs induced by the keys, with a protected constructor from Compare), node_type (a node
// handle with key_type / mapped_type / allocator_type), insert_return_type for map only,
// bidirectional iterators, reverse iterators, allocator_type, size/difference types.
// [container.reqmts]/66.4: iterator copy and move do not throw.
// COUNTERPART: libcxx:containers/associative/iterator_types.pass.cpp
#include <map>
#include <functional>
#include <type_traits>
#include "container_values.hpp"
#include "reqs/associative.hpp"
#include "reqs/iterator_nothrow.hpp"

using M = std::map<int, Elem>;
using MM = std::multimap<int, Elem>;
static_assert(std::is_same_v<M::key_type, int> && std::is_same_v<M::mapped_type, Elem>);
static_assert(std::is_same_v<M::value_type, std::pair<const int, Elem>>);
static_assert(std::is_same_v<M::key_compare, std::less<int>> && std::is_same_v<MM::key_compare, std::less<int>>);
static_assert(std::is_same_v<M::allocator_type, std::allocator<std::pair<const int, Elem>>>);
static_assert(std::is_same_v<M::pointer, std::pair<const int, Elem>*>);
static_assert(std::is_same_v<std::map<int, int, std::greater<int>>::key_compare, std::greater<int>>);

// value_compare: its constructor is protected; a derived class can reach it
struct Probe : M::value_compare {
  constexpr Probe() : M::value_compare(std::less<int>()) {}
};
static_assert(!std::is_constructible_v<M::value_compare, std::less<int>>);
static_assert(Probe()(M::value_type(1, Elem(9)), M::value_type(2, Elem(0))));
static_assert(!Probe()(M::value_type(2, Elem(0)), M::value_type(1, Elem(9))));
static_assert(!Probe()(M::value_type(1, Elem(0)), M::value_type(1, Elem(9))));  // keys only

static_assert(reqs::associative::types<std::map<int, int>>());
static_assert(reqs::associative::types<M>());
static_assert(reqs::associative::types<std::multimap<int, int>>());
static_assert(reqs::associative::types<MM>());
static_assert(reqs::associative::types<std::map<Elem, Elem, std::greater<>>>());
static_assert(reqs::iterator_nothrow::check<M>() && reqs::iterator_nothrow::check<MM>());
// map and multimap with the same key, mapped type and allocator have the same node_type
static_assert(std::is_same_v<std::map<int, Elem, std::greater<int>>::node_type, M::node_type>);
static_assert(std::is_same_v<MM::node_type, M::node_type>);

int main() { return 0; }
