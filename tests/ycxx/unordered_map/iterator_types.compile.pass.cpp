// The iterator types of the unordered containers. [container.reqmts]/5-9: X::difference_type is
// "identical to the difference type of X::iterator and X::const_iterator"; X::iterator's value
// type is T and its reference type T& (const T& for const_iterator), and iterator converts to
// const_iterator. [unord.req.general]/21-22: local_iterator is "An iterator type whose
// category, value type, difference type, and pointer and reference types are the same as
// X::iterator's", const_local_iterator the same as X::const_iterator's. /242 (iterators): at
// least forward iterators; with the key type as value type (the sets) iterator and
// const_iterator are both constant iterators. Checked through iterator_traits, for the default
// allocator and one with other size and difference types.
// COUNTERPART: libcxx:containers/unord/iterator_difference_type.pass.cpp
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

template <class T>
struct small_alloc {
  using value_type = T;
  using size_type = unsigned short;
  using difference_type = short;
  small_alloc() = default;
  template <class U> small_alloc(const small_alloc<U>&) {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  friend bool operator==(small_alloc, small_alloc) { return true; }
};

template <class It, class Ref>
constexpr bool same_traits() {
  using T = std::iterator_traits<It>;
  using R = std::iterator_traits<Ref>;
  return std::is_same_v<typename T::iterator_category, typename R::iterator_category> &&
         std::is_same_v<typename T::value_type, typename R::value_type> &&
         std::is_same_v<typename T::difference_type, typename R::difference_type> &&
         std::is_same_v<typename T::pointer, typename R::pointer> &&
         std::is_same_v<typename T::reference, typename R::reference>;
}

template <class X, class V, bool Constant>
constexpr bool check() {
  using I = typename X::iterator;
  using CI = typename X::const_iterator;
  using IT = std::iterator_traits<I>;
  using CIT = std::iterator_traits<CI>;
  static_assert(std::is_same_v<typename X::value_type, V>);
  static_assert(std::is_same_v<typename IT::difference_type, typename X::difference_type>);
  static_assert(std::is_same_v<typename CIT::difference_type, typename X::difference_type>);
  static_assert(std::is_same_v<typename IT::value_type, V>);
  static_assert(std::is_same_v<typename CIT::value_type, V>);
  static_assert(std::is_same_v<typename IT::reference, std::conditional_t<Constant, const V&, V&>>);
  static_assert(std::is_same_v<typename CIT::reference, const V&>);
  static_assert(std::forward_iterator<I> && std::forward_iterator<CI>);
  static_assert(std::is_convertible_v<I, CI>);
  static_assert(same_traits<typename X::local_iterator, I>());
  static_assert(same_traits<typename X::const_local_iterator, CI>());
  static_assert(std::forward_iterator<typename X::local_iterator>);
  return true;
}

using P = std::pair<const int, long>;
static_assert(check<std::unordered_map<int, long>, P, false>());
static_assert(check<std::unordered_multimap<int, long>, P, false>());
static_assert(check<std::unordered_set<int>, int, true>());
static_assert(check<std::unordered_multiset<int>, int, true>());

using H = std::hash<int>;
using E = std::equal_to<int>;
static_assert(check<std::unordered_map<int, long, H, E, small_alloc<P>>, P, false>());
static_assert(check<std::unordered_multimap<int, long, H, E, small_alloc<P>>, P, false>());
static_assert(check<std::unordered_set<int, H, E, small_alloc<int>>, int, true>());
static_assert(check<std::unordered_multiset<int, H, E, small_alloc<int>>, int, true>());

int main() {}
