// [associative.reqmts.general]/175: "The insert, insert_range, and emplace members shall not
// affect the validity of iterators and references to the container, and the erase members
// shall invalidate only iterators and references to the erased elements." The past-the-end
// iterator ([container.reqmts]/31: end() returns "An iterator which is the past-the-end value
// for the container") is one of those iterators, also when it was taken from a container that
// has never held an element. Every way of getting an empty map / multimap is covered:
// default-initialized, value-initialized, from a comparator or an allocator (/18-22), from an
// empty iterator range (/23-25), range (/32-34) or initializer list (/36), a copy of, moved
// from or assigned from a never-filled one, a = il (/37-40), swapped with one, and one that
// held elements and was cleared (/138-140), erased (/118-137) or extracted (/100-111) to
// empty. On each, end(), cend(), begin(), rbegin() and the iterators that failed lookups and
// erase(begin(), end()) returned are saved, then the first element(s) are put in by each
// insertion member: insert(t) (/61-69), insert(P&&) ([map.modifiers]/1-2,
// [multimap.modifiers]/1-2), insert(p, t) (/70-74), insert(i, j) (/75-78), insert_range (/79-82), insert(il) (/83), emplace (/47-56),
// emplace_hint (/57-60), insert(nh) and insert(p, nh) (/84-99), and for map also the
// insertion members of [map.modifiers]: try_emplace (/3-15) and insert_or_assign (/16-31),
// with and without a hint, and operator[] ([map.access]), which insert as well (their
// validity guarantee is the one /175 gives every insertion into an associative container).
// Afterwards the saved iterators are still the past-the-end iterator: equal to end(),
// std::prev of them is the last element, reverse iteration from reverse_iterator(e) visits
// every element, failed lookups (/141-171) return them, they work as hints (/58, /72: "as
// close as possible to the position just prior to p") and as the end of erase(begin(), e)
// (/134-137).
#include <map>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <ranges>
#include <tuple>
#include <utility>
#include "container_values.hpp"
#include "check.hpp"

template <class X>
constexpr typename X::key_type key(int i) {
  return val<typename X::key_type>(i);
}
template <class X>
constexpr typename X::value_type elem(int i) {
  return typename X::value_type(val<typename X::key_type>(i), val<typename X::mapped_type>(i));
}
template <class X>
constexpr typename X::mapped_type mapped(int i) {
  return val<typename X::mapped_type>(i);
}
template <class X>
constexpr bool is_map = requires(X& c) { c.try_emplace(key<X>(0)); };

constexpr int n_starts = 18;

// Calls f(c) on an empty container c obtained in the start state s.
template <class X, class F>
constexpr bool from_start(int s, F f) {
  using V = typename X::value_type;
  switch (s) {
    case 0: {
      X c;
      return f(c);
    }
    case 1: {
      X c{};
      return f(c);
    }
    case 2: {
      X c = X();
      return f(c);
    }
    case 3: {
      X c(typename X::key_compare{});
      return f(c);
    }
    case 4: {
      X c(typename X::allocator_type{});
      return f(c);
    }
    case 5: {
      V arr[1] = {elem<X>(0)};
      X c(arr, arr);
      return f(c);
    }
    case 6: {
      X c(std::from_range, std::ranges::empty_view<V>{});
      return f(c);
    }
    case 7: {
      X c(std::initializer_list<V>{});
      return f(c);
    }
    case 8: {
      const X d;
      X c(d);
      return f(c);
    }
    case 9: {
      X d;
      X c(std::move(d));
      return f(c);
    }
    case 10: {
      const X d;
      X c{elem<X>(0), elem<X>(1)};
      c = d;
      return f(c);
    }
    case 11: {
      X d;
      X c{elem<X>(0), elem<X>(1)};
      c = std::move(d);
      return f(c);
    }
    case 12: {
      X c{elem<X>(0), elem<X>(1)};
      c = std::initializer_list<V>{};
      return f(c);
    }
    case 13: {
      X d;
      X c{elem<X>(0), elem<X>(1)};
      c.swap(d);
      return f(c);
    }
    case 14: {
      X c{elem<X>(0), elem<X>(1), elem<X>(2)};
      c.clear();
      return f(c);
    }
    case 15: {
      X c{elem<X>(0), elem<X>(1), elem<X>(2)};
      c.erase(c.begin(), c.end());
      return f(c);
    }
    case 16: {
      X c{elem<X>(0), elem<X>(1), elem<X>(2)};
      c.erase(key<X>(1));
      c.erase(c.begin());
      c.erase(std::prev(c.cend()));
      return f(c);
    }
    case 17: {
      X c{elem<X>(0), elem<X>(1), elem<X>(2)};
      while (!c.empty()) (void)c.extract(c.begin());
      return f(c);
    }
  }
  return false;
}

template <class X>
constexpr int n_ops = is_map<X> ? 24 : 16;

// The first insertion into the empty c (keys 0-2 only); e and ce are c's end() and cend()
// taken before it.
template <class X>
constexpr void first_insertion(X& c, typename X::iterator e, typename X::const_iterator ce, int w) {
  using V = typename X::value_type;
  using K = typename X::key_type;
  using M = typename X::mapped_type;
  switch (w) {
    case 0: {
      const V v = elem<X>(1);
      c.insert(v);
      break;
    }
    case 1:
      c.insert(elem<X>(1));
      break;
    case 2:
      c.insert(std::pair<K, M>(key<X>(1), mapped<X>(1)));
      break;
    case 3:
      c.insert(e, elem<X>(1));
      break;
    case 4: {
      const V v = elem<X>(1);
      c.insert(ce, v);
      break;
    }
    case 5:
      c.insert(e, std::pair<K, M>(key<X>(1), mapped<X>(1)));
      break;
    case 6: {
      V arr[3] = {elem<X>(2), elem<X>(0), elem<X>(1)};
      c.insert(arr, arr + 3);
      break;
    }
    case 7:
      c.insert({elem<X>(2), elem<X>(0), elem<X>(1)});
      break;
    case 8: {
      V arr[3] = {elem<X>(2), elem<X>(0), elem<X>(1)};
      c.insert_range(arr);
      break;
    }
    case 9:
      c.emplace(key<X>(1), mapped<X>(1));
      break;
    case 10:
      c.emplace_hint(e, key<X>(1), mapped<X>(1));
      break;
    case 11:
      c.emplace_hint(ce, elem<X>(1));
      break;
    case 12: {
      X d{elem<X>(1)};
      c.insert(d.extract(d.begin()));
      break;
    }
    case 13: {
      X d{elem<X>(0), elem<X>(1)};
      c.insert(e, d.extract(key<X>(1)));
      break;
    }
    case 14: {
      X d{elem<X>(2)};
      c.insert(ce, d.extract(d.cbegin()));
      break;
    }
    case 15:
      c.emplace(std::piecewise_construct, std::forward_as_tuple(key<X>(1)), std::forward_as_tuple(mapped<X>(1)));
      break;
  }
  if constexpr (is_map<X>) {
    switch (w) {
      case 16:
        c.try_emplace(key<X>(1), mapped<X>(1));
        break;
      case 17: {
        const K k = key<X>(1);
        c.try_emplace(e, k, mapped<X>(1));
        break;
      }
      case 18:
        c.try_emplace(ce, key<X>(1), mapped<X>(1));
        break;
      case 19: {
        const K k = key<X>(1);
        c.insert_or_assign(k, mapped<X>(1));
        break;
      }
      case 20:
        c.insert_or_assign(e, key<X>(1), mapped<X>(1));
        break;
      case 21:
        c.insert_or_assign(ce, key<X>(1), mapped<X>(1));
        break;
      case 22:
        c[key<X>(1)] = mapped<X>(1);
        break;
      case 23: {
        const K k = key<X>(1);
        c[k] = mapped<X>(1);
        break;
      }
    }
  }
}

// e / ce are the past-the-end iterator of the non-empty c (holding keys below 50).
template <class X>
constexpr bool end_usable(X& c, typename X::iterator e, typename X::const_iterator ce) {
  using It = typename X::iterator;
  using RIt = typename X::reverse_iterator;
  if (c.empty()) return false;
  if (e != c.end() || ce != c.cend() || e != c.cend() || ce != c.end() || std::as_const(c).end() != ce) return false;
  // forward to e, backward from e
  std::size_t n = 0;
  It last = e;
  for (It it = c.begin(); it != e; ++it, ++n) last = it;
  if (n != c.size() || std::prev(e) != last || std::prev(ce) != last) return false;
  n = 0;
  for (It it = e; it != c.begin(); ++n) --it;
  if (n != c.size()) return false;
  RIt r(e);
  if (r != c.rbegin() || std::addressof(*r) != std::addressof(*last)) return false;
  n = 0;
  for (; r != c.rend(); ++r) ++n;
  if (n != c.size()) return false;
  // failed lookups
  if (c.find(key<X>(60)) != e || c.lower_bound(key<X>(60)) != e || c.upper_bound(key<X>(60)) != ce) return false;
  if (c.equal_range(key<X>(60)).first != e || c.equal_range(key<X>(60)).second != e) return false;
  if (std::as_const(c).find(key<X>(60)) != ce) return false;
  // as hints: the new largest elements go just before e
  const auto* first_addr = std::addressof(*c.begin());
  It h = c.insert(e, elem<X>(50));
  if (!(*h == elem<X>(50)) || std::next(h) != e || e != c.end() || std::prev(e) != h) return false;
  It h2 = c.emplace_hint(ce, elem<X>(51));
  if (!(*h2 == elem<X>(51)) || std::next(h2) != e || std::prev(h2) != h || ce != c.cend()) return false;
  if (std::addressof(*c.begin()) != first_addr) return false;
  if constexpr (is_map<X>) {
    It h3 = c.try_emplace(e, key<X>(52), mapped<X>(52));
    if (!(*h3 == elem<X>(52)) || std::next(h3) != e || std::prev(h3) != h2) return false;
    It h4 = c.insert_or_assign(ce, key<X>(53), mapped<X>(53));
    if (!(*h4 == elem<X>(53)) || std::next(h4) != e || std::prev(h4) != h3 || e != c.end()) return false;
  }
  // as the end of an erased range
  if (c.erase(c.begin(), e) != e || !c.empty() || c.begin() != e || c.end() != e || c.cend() != ce) return false;
  // and once more after emptying
  c.insert(elem<X>(5));
  if (c.end() != e || std::prev(e) != c.begin() || !(*std::prev(e) == elem<X>(5))) return false;
  if (c.erase(c.cbegin()) != e || !c.empty()) return false;
  return true;
}

template <class X>
constexpr bool one(int s, int w) {
  return from_start<X>(s, [w](X& c) {
    using It = typename X::iterator;
    using CIt = typename X::const_iterator;
    if (!c.empty() || c.begin() != c.end()) return false;
    const It e = c.end();
    const CIt ce = c.cend();
    const It b = c.begin();
    const CIt cb = std::as_const(c).begin();
    const It f = c.find(key<X>(9));
    const It lb = c.lower_bound(key<X>(9));
    const It ub = c.upper_bound(key<X>(9));
    const auto eq = c.equal_range(key<X>(9));
    const It er = c.erase(c.begin(), c.end());
    const typename X::reverse_iterator rb = c.rbegin();
    first_insertion(c, e, ce, w);
    if (c.empty()) return false;
    // every iterator saved from the never-filled container is still the past-the-end one
    if (b != c.end() || cb != c.cend() || f != c.end() || lb != c.end() || ub != c.end() || eq.first != c.end() ||
        eq.second != c.end() || er != c.end())
      return false;
    if (rb != c.rbegin() || std::addressof(*rb) != std::addressof(*std::prev(c.end()))) return false;
    return end_usable(c, e, ce);
  });
}

template <class X>
constexpr bool test() {
  for (int s = 0; s < n_starts; ++s)
    for (int w = 0; w < n_ops<X>; ++w)
      if (!one<X>(s, w)) return false;
  return true;
}

static_assert(test<std::map<int, int>>());
static_assert(test<std::map<int, Elem>>());
static_assert(test<std::map<Elem, Elem>>());
static_assert(test<std::multimap<int, int>>());
static_assert(test<std::multimap<Elem, Elem>>());

template <class X>
void run(const char* name) {
  for (int s = 0; s < n_starts; ++s)
    for (int w = 0; w < n_ops<X>; ++w) CHECK_SAY(one<X>(s, w), "%s: start state %d, insertion %d", name, s, w);
}

int main() {
  run<std::map<int, int>>("map<int, int>");
  run<std::map<int, Elem>>("map<int, Elem>");
  run<std::map<Elem, Elem>>("map<Elem, Elem>");
  run<std::map<long, int, std::less<>>>("map<long, int, less<>>");
  run<std::multimap<int, int>>("multimap<int, int>");
  run<std::multimap<Elem, Elem>>("multimap<Elem, Elem>");
  return 0;
}
