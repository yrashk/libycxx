// [flat.map.defn], [flat.multimap.defn]: value_type is pair<Key, T> (not pair<const Key,
// T>), reference is pair<const Key&, T&>, const_reference pair<const Key&, const T&>,
// size_type size_t, difference_type ptrdiff_t, key_container_type / mapped_container_type
// default to vector<Key> / vector<T>, the nested aggregate `containers` with members keys and
// values; [flat.map.overview]/1: the iterators model random_access_iterator; keys() and
// values() return const references to the underlying containers. [flat.map.syn]:
// sorted_unique_t / sorted_equivalent_t tags and uses_allocator specializations.
// [flat.map.overview]/8: Key / T must match the containers' value types.
#include <flat_map>
#include <deque>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

using M = std::flat_map<int, double>;
using MM = std::flat_multimap<int, double, std::greater<int>, std::deque<int>, std::deque<double>>;
static_assert(std::is_same_v<M::value_type, std::pair<int, double>>);
static_assert(std::is_same_v<M::reference, std::pair<const int&, double&>>);
static_assert(std::is_same_v<M::const_reference, std::pair<const int&, const double&>>);
static_assert(std::is_same_v<M::size_type, std::size_t> && std::is_same_v<M::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<M::key_container_type, std::vector<int>>);
static_assert(std::is_same_v<M::mapped_container_type, std::vector<double>>);
static_assert(std::is_same_v<M::key_compare, std::less<int>>);
static_assert(std::is_same_v<MM::key_container_type, std::deque<int>> && std::is_same_v<MM::key_compare, std::greater<int>>);
static_assert(std::random_access_iterator<M::iterator> && std::random_access_iterator<M::const_iterator>);
static_assert(std::random_access_iterator<MM::iterator>);
static_assert(std::is_same_v<std::iter_reference_t<M::iterator>, M::reference>);
static_assert(std::is_same_v<std::iter_reference_t<M::const_iterator>, M::const_reference>);
static_assert(std::is_convertible_v<M::iterator, M::const_iterator>);
static_assert(std::is_same_v<M::reverse_iterator, std::reverse_iterator<M::iterator>>);
static_assert(std::is_same_v<decltype(std::declval<const M&>().keys()), const std::vector<int>&>);
static_assert(std::is_same_v<decltype(std::declval<const M&>().values()), const std::vector<double>&>);
static_assert(noexcept(std::declval<const M&>().keys()) && noexcept(std::declval<const M&>().values()));
static_assert(std::is_aggregate_v<M::containers>);
static_assert(std::is_same_v<decltype(M::containers::keys), std::vector<int>>);
static_assert(std::is_same_v<decltype(M::containers::values), std::vector<double>>);
static_assert(std::is_same_v<decltype(std::declval<M&&>().extract()), M::containers>);
static_assert(std::is_same_v<decltype(std::sorted_unique), const std::sorted_unique_t>);
static_assert(std::is_same_v<decltype(std::sorted_equivalent), const std::sorted_equivalent_t>);
static_assert(std::is_default_constructible_v<std::sorted_unique_t> && std::is_empty_v<std::sorted_unique_t>);
static_assert(std::uses_allocator_v<M, std::allocator<int>>);
static_assert(std::uses_allocator_v<MM, std::allocator<double>>);
template <class X>
concept has_node_type = requires { typename X::node_type; };
template <class X>
concept has_allocator_type = requires { typename X::allocator_type; };
static_assert(!has_node_type<M> && !has_allocator_type<M>);  // not allocator-aware, no node handles
static_assert(std::is_invocable_r_v<bool, M::value_compare, M::const_reference, M::const_reference>);
static_assert(!std::is_default_constructible_v<M::value_compare>);

int main() { return 0; }
