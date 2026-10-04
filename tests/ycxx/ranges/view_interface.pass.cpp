// [view.interface]: the members view_interface<D> provides and their constraints: empty()
// (sized or forward), operator bool, data() (contiguous iterator), size() (forward with a
// sized sentinel), front() (forward), back() (bidirectional and common), operator[]
// (random access), cbegin()/cend() (input).
#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;

// A view over a pointer range whose iterator/sentinel types are chosen by the test.
template <class It, class Sent = It>
struct V : rg::view_interface<V<It, Sent>> {
  It b{};
  Sent e{};
  V() = default;
  constexpr V(It x, Sent y) : b(x), e(y) {}
  constexpr It begin() const { return b; }
  constexpr Sent end() const { return e; }
};

template <class T>
concept has_empty = requires(T& t) { t.empty(); };
template <class T>
concept has_bool = requires(T& t) { static_cast<bool>(t); };
template <class T>
concept has_data = requires(T& t) { t.data(); };
template <class T>
concept has_size = requires(T& t) { t.size(); };
template <class T>
concept has_front = requires(T& t) { t.front(); };
template <class T>
concept has_back = requires(T& t) { t.back(); };
template <class T>
concept has_subscript = requires(T& t) { t[0]; };
template <class T>
concept has_cbegin = requires(T& t) { t.cbegin(); t.cend(); };

using Contig = V<int*>;
static_assert(has_empty<Contig> && has_bool<Contig> && has_data<Contig> && has_size<Contig>);
static_assert(has_front<Contig> && has_back<Contig> && has_subscript<Contig> && has_cbegin<Contig>);
static_assert(has_empty<const Contig> && has_data<const Contig> && has_subscript<const Contig>);
static_assert(std::is_same_v<decltype(std::declval<Contig&>().data()), int*>);
static_assert(std::is_same_v<decltype(std::declval<Contig&>().front()), int&>);
static_assert(std::is_same_v<decltype(std::declval<Contig&>()[1]), int&>);
static_assert(!std::is_convertible_v<Contig, bool>); // explicit operator bool

using Rand = V<RandomIter<int>>;
static_assert(has_size<Rand> && has_subscript<Rand> && has_back<Rand> && !has_data<Rand>);

using RandNC = V<RandomIter<int>, PtrSentinel<int>>; // not common, not sized
static_assert(!has_size<RandNC> && !has_back<RandNC> && has_subscript<RandNC> && has_empty<RandNC>);

using Bidi = V<BidiIter<int>>;
static_assert(has_back<Bidi> && has_front<Bidi> && !has_subscript<Bidi> && !has_size<Bidi> && has_empty<Bidi>);

using Fwd = V<ForwardIter<int>>;
static_assert(has_front<Fwd> && !has_back<Fwd> && has_empty<Fwd> && has_bool<Fwd> && !has_size<Fwd>);

using In = V<InputIter<int>, PtrSentinel<int>>;
static_assert(!has_empty<In> && !has_bool<In> && !has_front<In> && has_cbegin<In>);
static_assert(rg::input_range<In>);

constexpr bool test() {
  int a[5] = {1, 2, 3, 4, 5};
  Contig c(a, a + 5);
  CHECK(!c.empty() && bool(c) && c.data() == a && c.size() == 5 && c.front() == 1 && c.back() == 5 && c[2] == 3);
  const Contig& cc = c;
  CHECK(cc.size() == 5 && cc[4] == 5 && cc.data() == a);
  CHECK(*c.cbegin() == 1 && c.cend() - c.cbegin() == 5);
  static_assert(std::is_same_v<decltype(*c.cbegin()), const int&>);
  Contig e(a, a);
  CHECK(e.empty() && !e && e.size() == 0);
  Rand r(RandomIter<int>(a), RandomIter<int>(a + 5));
  CHECK(r.size() == 5 && r.back() == 5 && r[3] == 4);
  Bidi bd(BidiIter<int>(a), BidiIter<int>(a + 3));
  CHECK(bd.back() == 3 && bd.front() == 1 && !bd.empty());
  Fwd f(ForwardIter<int>(a + 2), ForwardIter<int>(a + 2));
  CHECK(f.empty() && !f);
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
