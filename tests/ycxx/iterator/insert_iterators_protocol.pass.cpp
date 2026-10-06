// [back.insert.iter.ops]/1-7, [front.insert.iter.ops]/1-7, [insert.iter.ops]/1-7, with a container
// that records which member is called: assignment from a const lvalue calls
// push_back(value) / push_front(value) / insert(iter, value), from an rvalue the
// std::move(value) overload; insert_iterator then sets iter to the result of insert and
// increments it, so successive assignments insert in order; *, ++ and ++(int) return *this
// (insert_iterator's postfix ++ returns a reference, the others' a copy). The container is held
// by addressof (an overloaded unary & is not used). The class definitions
// ([back.insert.iterator] etc.): container_type, output_iterator_tag, void value_type, pointer
// and reference, ptrdiff_t difference_type. All constexpr: checked in constant evaluation too.
#include <cstddef>
#include <iterator>
#include <list>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

struct Elem {
  int v;
  constexpr bool operator==(const Elem&) const = default;
};

struct Rec {
  using value_type = Elem;
  using iterator = std::vector<Elem>::iterator;
  std::vector<Elem> data;
  int copies = 0, moves = 0;
  constexpr void push_back(const Elem& e) { ++copies; data.push_back(e); }
  constexpr void push_back(Elem&& e) { ++moves; data.push_back(e); }
  constexpr void push_front(const Elem& e) { ++copies; data.insert(data.begin(), e); }
  constexpr void push_front(Elem&& e) { ++moves; data.insert(data.begin(), e); }
  constexpr iterator insert(iterator pos, const Elem& e) { ++copies; return data.insert(pos, e); }
  constexpr iterator insert(iterator pos, Elem&& e) { ++moves; return data.insert(pos, e); }
  constexpr iterator begin() { return data.begin(); }
  constexpr iterator end() { return data.end(); }
  void operator&() const = delete; // addressof must be used
};

using BI = std::back_insert_iterator<Rec>;
using FI = std::front_insert_iterator<Rec>;
using II = std::insert_iterator<Rec>;
template <class It>
constexpr bool traits_ok = std::is_same_v<typename It::container_type, Rec> &&
                           std::is_same_v<typename It::iterator_category, std::output_iterator_tag> &&
                           std::is_void_v<typename It::value_type> && std::is_void_v<typename It::pointer> &&
                           std::is_void_v<typename It::reference> &&
                           std::is_same_v<typename It::difference_type, std::ptrdiff_t>;
static_assert(traits_ok<BI> && traits_ok<FI> && traits_ok<II>);
static_assert(std::is_same_v<decltype(std::declval<BI&>()++), BI> && std::is_same_v<decltype(std::declval<FI&>()++), FI>);
static_assert(std::is_same_v<decltype(std::declval<II&>()++), II&>);
static_assert(std::is_same_v<decltype(*std::declval<II&>()), II&> && std::is_same_v<decltype(++std::declval<BI&>()), BI&>);
static_assert(!std::is_convertible_v<Rec&, BI> && !std::is_convertible_v<Rec&, FI>);
static_assert(std::output_iterator<BI, Elem> && std::output_iterator<II, const Elem&>);

constexpr bool run() {
  Rec r;
  const Elem one{1};
  BI b(r);
  if (&(b = one) != &b || &*b != &b || &++b != &b) return false;
  b = Elem{2};
  BI old = b++;
  old = Elem{3};
  if (r.copies != 1 || r.moves != 2 || r.data != std::vector<Elem>{{1}, {2}, {3}}) return false;

  FI f(r);
  if (&(f = one) != &f || &*f != &f) return false;
  f = Elem{0};
  if (r.copies != 2 || r.moves != 3 || r.data.front() != Elem{0} || r.data[1] != Elem{1}) return false;

  // insert_iterator: inserts before the position it was made with, in order.
  Rec q;
  q.data = {{10}, {40}};
  II i(q, q.begin() + 1);
  i = Elem{20};
  const Elem thirty{30};
  if (&(i = thirty) != &i || &i++ != &i || &++i != &i || &*i != &i) return false;
  if (q.data != std::vector<Elem>{{10}, {20}, {30}, {40}} || q.moves != 1 || q.copies != 1) return false;
  // inserter at the end, and the helper functions.
  auto e = std::inserter(q, q.end());
  e = Elem{50};
  std::back_inserter(q) = Elem{60};
  std::front_inserter(q) = Elem{5};
  if (q.data.front() != Elem{5} || q.data.back() != Elem{60} || q.data.size() != 7) return false;
  return true;
}
static_assert(run());

int main() {
  CHECK(run());
  // With a standard container: insert_iterator into a list keeps its position.
  std::list<int> l{1, 5};
  auto it = std::inserter(l, std::next(l.begin()));
  for (int v : {2, 3, 4}) *it++ = v;
  CHECK((l == std::list<int>{1, 2, 3, 4, 5}));
  return 0;
}
