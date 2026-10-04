// [back.insert.iterator], [front.insert.iterator], [insert.iterator]: member types
// (output_iterator_tag, value_type/pointer/reference void, difference_type ptrdiff_t,
// container_type); the protected member `container` (and `iter` for insert_iterator) is
// initialized with addressof(x) (so an overloaded unary & of the container is not used).
// operator=(const value_type&) is "container->push_back(value)" / push_front(value) / "iter =
// container->insert(iter, value); ++iter;"; operator=(value_type&&) the same with
// std::move(value). *it and ++it return *this; it++ returns *this by value for
// back/front_insert_iterator and by reference for insert_iterator. back_inserter(x),
// front_inserter(x), inserter(x, i) return the corresponding iterator ([back.inserter],
// [front.inserter], [inserter]). Everything is constexpr. The iterators model
// output_iterator ([iterator.concept.output]); insert_iterator's iter has type
// ranges::iterator_t<Container>.
#include <iterator>
#include <algorithm>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

struct Elem {
  int v = 0;
  bool moved_from = false;
  constexpr Elem() = default;
  constexpr Elem(int x) : v(x) {}
  constexpr Elem(const Elem& o) : v(o.v) {}
  constexpr Elem(Elem&& o) : v(o.v) { o.moved_from = true; }
  constexpr Elem& operator=(const Elem& o) {
    v = o.v;
    return *this;
  }
  constexpr Elem& operator=(Elem&& o) {
    v = o.v;
    o.moved_from = true;
    return *this;
  }
};

// A minimal sequence with a deleted unary &, recording which overloads were called.
struct Seq {
  using value_type = Elem;
  using const_reference = const Elem&;
  Elem data[32];
  int n = 0;
  int copies = 0, moves = 0, ins_copies = 0, ins_moves = 0;
  constexpr Elem* begin() { return data; }
  constexpr Elem* end() { return data + n; }
  constexpr void push_back(const Elem& e) {
    ++copies;
    data[n++] = e;
  }
  constexpr void push_back(Elem&& e) {
    ++moves;
    data[n++] = std::move(e);
  }
  constexpr void push_front(const Elem& e) {
    ++copies;
    insert(data, e);
  }
  constexpr void push_front(Elem&& e) {
    ++moves;
    insert(data, std::move(e));
  }
  template <class E>
  constexpr Elem* insert(Elem* pos, E&& e) {
    if constexpr (std::is_lvalue_reference_v<E>)
      ++ins_copies;
    else
      ++ins_moves;
    for (Elem* p = data + n; p != pos; --p) p[0] = p[-1];
    *pos = std::forward<E>(e);
    ++n;
    return pos;
  }
  void operator&() const = delete;
  constexpr bool is(std::initializer_list<int> il) const {
    if (static_cast<std::size_t>(n) != il.size()) return false;
    int i = 0;
    for (int x : il)
      if (data[i++].v != x) return false;
    return true;
  }
};

template <class It>
constexpr bool types() {
  return std::is_same_v<typename It::iterator_category, std::output_iterator_tag> &&
         std::is_same_v<typename It::value_type, void> && std::is_same_v<typename It::difference_type, std::ptrdiff_t> &&
         std::is_same_v<typename It::pointer, void> && std::is_same_v<typename It::reference, void> &&
         std::is_same_v<typename It::container_type, Seq>;
}
static_assert(types<std::back_insert_iterator<Seq>>());
static_assert(types<std::front_insert_iterator<Seq>>());
static_assert(types<std::insert_iterator<Seq>>());
static_assert(std::output_iterator<std::back_insert_iterator<Seq>, const Elem&>);
static_assert(std::output_iterator<std::front_insert_iterator<Seq>, Elem&&>);
static_assert(std::output_iterator<std::insert_iterator<Seq>, const Elem&>);
static_assert(!std::is_convertible_v<Seq&, std::back_insert_iterator<Seq>>);  // explicit
static_assert(!std::is_convertible_v<Seq&, std::front_insert_iterator<Seq>>);
static_assert(std::is_same_v<decltype(std::declval<std::back_insert_iterator<Seq>&>()++), std::back_insert_iterator<Seq>>);
static_assert(std::is_same_v<decltype(std::declval<std::front_insert_iterator<Seq>&>()++), std::front_insert_iterator<Seq>>);
static_assert(std::is_same_v<decltype(std::declval<std::insert_iterator<Seq>&>()++), std::insert_iterator<Seq>&>);
static_assert(std::is_same_v<decltype(std::back_inserter(std::declval<Seq&>())), std::back_insert_iterator<Seq>>);
static_assert(std::is_same_v<decltype(std::inserter(std::declval<Seq&>(), nullptr)), std::insert_iterator<Seq>>);

// the protected members, as declared
struct PeekBack : std::back_insert_iterator<Seq> {
  using back_insert_iterator::back_insert_iterator;
  constexpr Seq* get() const { return container; }
};
struct PeekIns : std::insert_iterator<Seq> {
  using insert_iterator::insert_iterator;
  constexpr Seq* get() const { return container; }
  constexpr Elem* pos() const { return iter; }
};
static_assert(std::is_same_v<decltype(std::declval<PeekIns&>().pos()), std::ranges::iterator_t<Seq>>);

constexpr bool test() {
  {
    Seq s;
    auto it = std::back_inserter(s);
    if (&*it != std::addressof(it) || &++it != std::addressof(it)) return false;
    Elem e(1);
    it = e;  // copy
    if (e.moved_from || s.copies != 1) return false;
    *it++ = Elem(2);  // move
    it = std::move(e);
    if (!e.moved_from || s.moves != 2) return false;
    if (!s.is({1, 2, 1})) return false;
    PeekBack pb(s);
    if (pb.get() != std::addressof(s)) return false;
  }
  {
    Seq s;
    auto it = std::front_inserter(s);
    Elem e(1);
    it = e;
    *it++ = Elem(2);
    ++it = Elem(3);
    if (!s.is({3, 2, 1}) || s.copies != 1 || s.moves != 2 || e.moved_from) return false;
  }
  {
    Seq s;
    s.push_back(Elem(1));
    s.push_back(Elem(5));
    s.copies = s.moves = s.ins_copies = s.ins_moves = 0;
    auto it = std::inserter(s, s.begin() + 1);
    Elem e(2);
    it = e;  // inserted before 5, iter then points to 5 again
    *it++ = Elem(3);
    it = Elem(4);
    if (!s.is({1, 2, 3, 4, 5}) || e.moved_from) return false;
    if (s.ins_copies != 1 || s.ins_moves != 2 || s.copies != 0 || s.moves != 0) return false;
    PeekIns pi(s, s.begin() + 2);
    if (pi.get() != std::addressof(s) || pi.pos() != s.begin() + 2) return false;
  }
  {
    // with algorithms and std::vector (constexpr)
    std::vector<int> v;
    int src[] = {1, 2, 3};
    std::copy(src, src + 3, std::back_inserter(v));
    std::ranges::copy(src, std::inserter(v, v.begin() + 1));
    if (v.size() != 6 || v[0] != 1 || v[1] != 1 || v[3] != 3 || v[4] != 2 || v[5] != 3) return false;
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
  return 0;
}
