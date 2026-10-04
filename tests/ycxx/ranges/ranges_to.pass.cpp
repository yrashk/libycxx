// [range.utility.conv.to]: ranges::to<C> picks, in order, C(r, args...) (/2.1.1),
// C(from_range, r, args...) (/2.1.2), C(begin, end, args...) for common ranges (/2.1.3), and
// default construction plus emplace_back / push_back / insert (emplace_hint: see
// ranges_to_emplace_hint), reserving
// first for reservable containers (/2.1.4); it recurses for ranges of ranges (/2.2); the
// template-template form deduces C (/3-/5); ranges::to<C>(args...) is a closure
// ([range.utility.conv.adaptors]).
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

enum class How { none, range, from_range, iters, emplace_back, push_back, emplace_hint, insert };

// A small fixed-capacity container base; derived classes add exactly one way of filling it.
struct Store {
  int data[16] = {};
  std::size_t n = 0;
  How how = How::none;
  int extra = 0;
  std::size_t reserved = 0;
  constexpr int* begin() { return data; }
  constexpr int* end() { return data + n; }
  constexpr const int* begin() const { return data; }
  constexpr const int* end() const { return data + n; }
  constexpr void add(int v) { data[n++] = v; }
};

struct ByRange : Store {
  template <class R>
  constexpr ByRange(R&& r, int x) {
    for (auto&& v : r) add(v);
    how = How::range;
    extra = x;
  }
};
struct ByFromRange : Store {
  template <class R>
  constexpr ByFromRange(std::from_range_t, R&& r) {
    for (auto&& v : r) add(v);
    how = How::from_range;
  }
};
struct ByIters : Store {
  ByIters() = default;
  template <class I>
  constexpr ByIters(I f, I l) {
    for (; f != l; ++f) add(*f);
    how = How::iters;
  }
  constexpr void push_back(int v) {
    add(v);
    how = How::push_back;
  }
};
struct ByEmplaceBack : Store {
  constexpr void emplace_back(int v) {
    add(v);
    how = How::emplace_back;
  }
  constexpr void push_back(int v) {
    add(v);
    how = How::push_back;
  }
  constexpr std::size_t size() const { return n; }
  constexpr void reserve(std::size_t k) { reserved = k; }
  constexpr std::size_t capacity() const { return 16; }
  constexpr std::size_t max_size() const { return 16; }
};
struct ByInsert : Store {
  explicit constexpr ByInsert(int x) { extra = x; }
  constexpr int* insert(int*, int v) {
    add(v);
    how = How::insert;
    return data;
  }
};

// Deduction for the template-template form.
template <class T>
struct Box {
  T items[8] = {};
  std::size_t n = 0;
  constexpr Box() = default;
  template <class R>
  constexpr Box(std::from_range_t, R&& r) {
    for (auto&& v : r) items[n++] = v;
  }
  constexpr T* begin() { return items; }
  constexpr T* end() { return items + n; }
  constexpr const T* begin() const { return items; }
  constexpr const T* end() const { return items + n; }
};
template <class R>
Box(std::from_range_t, R&&) -> Box<rg::range_value_t<R>>;

static_assert(std::is_same_v<decltype(rg::to<Box>(std::declval<long (&)[2]>())), Box<long>>);

template <class C, class R>
concept to_ok = requires(R&& r) { rg::to<C>(std::forward<R>(r)); };
static_assert(!to_ok<rg::ref_view<int[2]>, int (&)[2]>); // requires !view<C>

constexpr bool test() {
  int a[4] = {1, 2, 3, 4};
  {
    auto c = rg::to<ByRange>(a, 7);
    CHECK(c.how == How::range && c.extra == 7 && c.n == 4 && c.data[3] == 4);
  }
  {
    auto c = a | rg::to<ByFromRange>();
    CHECK(c.how == How::from_range && c.n == 4);
  }
  {
    auto c = rg::to<ByIters>(a);
    CHECK(c.how == How::iters && c.n == 4);
    // Not common: falls back to push_back.
    auto t = vw::iota(1) | vw::take(3) | rg::to<ByIters>();
    CHECK(t.how == How::push_back && t.n == 3 && t.data[2] == 3);
  }
  {
    auto c = rg::to<ByEmplaceBack>(a);
    CHECK(c.how == How::emplace_back && c.n == 4 && c.reserved == 4);
  }
  {
    auto c = a | rg::to<ByInsert>(9);
    CHECK(c.how == How::insert && c.extra == 9 && c.n == 4);
  }
  {
    auto b = vw::iota(0, 3) | vw::transform([](int x) { return x * 2L; }) | rg::to<Box>();
    static_assert(std::is_same_v<decltype(b), Box<long>>);
    CHECK(b.n == 3 && b.items[2] == 4);
  }
  {
    // Ranges of ranges: each element converted with to<range_value_t<C>> (/2.2).
    int m[2][3] = {{1, 2, 3}, {4, 5, 6}};
    auto nested = rg::to<Box<Box<int>>>(m);
    CHECK(nested.n == 2 && nested.items[1].n == 3 && nested.items[1].items[2] == 6);
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
