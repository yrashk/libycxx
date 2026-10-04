// [flat.set.defn], [flat.multiset.defn]: key_type = value_type = Key, value_compare =
// key_compare = Compare, size_type / difference_type from KeyContainer, container_type
// (default vector<Key>), random access constant iterators ([flat.set.overview]/1, the
// set's iterators are constant: [associative.reqmts.general]/6), no node handles, no
// allocator_type; uses_allocator specializations; extract() && returns container_type.
#include <flat_set>
#include <deque>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

using S = std::flat_set<int>;
using MS = std::flat_multiset<long, std::greater<>, std::deque<long>>;
static_assert(std::is_same_v<S::key_type, int> && std::is_same_v<S::value_type, int>);
static_assert(std::is_same_v<S::value_compare, std::less<int>> && std::is_same_v<MS::value_compare, std::greater<>>);
static_assert(std::is_same_v<S::container_type, std::vector<int>> && std::is_same_v<MS::container_type, std::deque<long>>);
static_assert(std::is_same_v<S::size_type, std::vector<int>::size_type>);
static_assert(std::is_same_v<MS::difference_type, std::deque<long>::difference_type>);
static_assert(std::is_same_v<S::reference, int&> && std::is_same_v<S::const_reference, const int&>);
static_assert(std::random_access_iterator<S::iterator> && std::random_access_iterator<MS::const_iterator>);
static_assert(std::is_same_v<std::iter_reference_t<S::iterator>, const int&>);
static_assert(!std::indirectly_writable<S::iterator, int>);
static_assert(std::is_same_v<decltype(std::declval<S&&>().extract()), std::vector<int>>);
static_assert(std::uses_allocator_v<S, std::allocator<int>> && !std::uses_allocator_v<S, std::allocator<char>> == false);
template <class X>
concept has_node_type = requires { typename X::node_type; };
static_assert(!has_node_type<S> && !has_node_type<MS>);

int main() { return 0; }
