// The range members of the sequence containers with a contiguous, sized source range whose
// element type differs from T but has the same size and is trivially copyable (int -> float,
// int -> unsigned, float -> int, int -> a trivially copyable wrapper with a transforming
// constructor). [sequence.reqmts]/11-14 X(from_range, rg) "Constructs a sequence container
// equal to the range rg" with T Cpp17EmplaceConstructible from *ranges::begin(rg); /40-43
// insert_range(p, rg) "Inserts copies of elements in rg before p"; /60-64 assign_range(rg)
// "Replaces elements in a with a copy of each element in rg"; /109-111 append_range;
// prepend_range ([deque.modifiers], [list.modifiers]). Each element is T(e) for the source
// element e -- a conversion, not a copy of e's bytes. Also with a span, a const range and a
// contiguous subrange, and for the string range members ([string.cons], [string.append],
// [string.insert], [string.replace] replace_with_range).
#include <vector>
#include <deque>
#include <list>
#include <string>
#include <inplace_vector>
#include <span>
#include <ranges>
#include <cstddef>
#include "check.hpp"

struct Twice {
  int v;
  Twice(int x) : v(2 * x + 1) {}
  Twice(float x) : v(static_cast<int>(x) * 3) {}
  friend bool operator==(Twice, Twice) = default;
};

template <class C, class E>
static bool equal(const C& c, const E& e) {
  if (c.size() != e.size()) return false;
  auto it = c.begin();
  for (std::size_t i = 0; i < e.size(); ++i, ++it)
    if (!(*it == e[i])) return false;
  return true;
}

template <class C, class S>
static void run_one(const std::vector<S>& srcv) {
  using T = typename C::value_type;
  std::vector<T> all;
  for (S s : srcv) all.push_back(static_cast<T>(s));
  const std::span<const S> sp(srcv);
  auto sub = std::ranges::subrange(srcv.data() + 1, srcv.data() + srcv.size());
  std::vector<T> tail(all.begin() + 1, all.end());

  CHECK(equal(C(std::from_range, srcv), all));
  CHECK(equal(C(std::from_range, sp), all));
  CHECK(equal(C(std::from_range, sub), tail));
  for (std::size_t init : {std::size_t{0}, std::size_t{2}, srcv.size() + 5}) {
    C c(init, static_cast<T>(S{}));
    c.assign_range(sp);
    CHECK(equal(c, all));
    std::vector<T> exp(init, static_cast<T>(S{}));
    C d(init, static_cast<T>(S{}));
    d.append_range(srcv);
    exp.insert(exp.end(), all.begin(), all.end());
    CHECK(equal(d, exp));
    if constexpr (requires { d.prepend_range(sp); }) {
      C e(init, static_cast<T>(S{}));
      e.prepend_range(sub);
      std::vector<T> pexp = tail;
      pexp.insert(pexp.end(), init, static_cast<T>(S{}));
      CHECK(equal(e, pexp));
    }
    for (std::size_t pos = 0; pos <= init; ++pos) {
      C f(init, static_cast<T>(S{}));
      if constexpr (requires { f.reserve(0u); }) {
        if (pos % 2) f.reserve(init + srcv.size() + 3);
      }
      auto p = f.begin();
      std::ranges::advance(p, static_cast<std::ptrdiff_t>(pos));
      auto r = f.insert_range(p, sp);
      CHECK(std::ranges::distance(f.begin(), r) == static_cast<std::ptrdiff_t>(pos));
      std::vector<T> iexp(init, static_cast<T>(S{}));
      iexp.insert(iexp.begin() + static_cast<std::ptrdiff_t>(pos), all.begin(), all.end());
      CHECK(equal(f, iexp));
    }
  }
}

template <template <class> class C>
static void run_all() {
  const std::vector<int> ints = {0, 1, -1, 16777217, 2147483647, -2147483647 - 1, 300, 1065353216};
  const std::vector<float> flts = {0.5f, -1.75f, 3.99f, 1e9f, -2e9f, 1.0f};
  run_one<C<float>>(ints);
  run_one<C<unsigned>>(ints);
  run_one<C<Twice>>(ints);
  run_one<C<int>>(flts);
  run_one<C<Twice>>(flts);
  run_one<C<double>>(flts);
  run_one<C<long long>>(ints);
}

template <class T>
using Vec = std::vector<T>;
template <class T>
using Deq = std::deque<T>;
template <class T>
using Lst = std::list<T>;
template <class T>
using IV = std::inplace_vector<T, 40>;

template <class Ch, class S>
static void run_string(const std::vector<S>& src) {
  using Str = std::basic_string<Ch>;
  Str exp;
  for (S s : src) exp.push_back(static_cast<Ch>(s));
  CHECK(Str(std::from_range, src) == exp);
  for (std::size_t len : {std::size_t{0}, std::size_t{3}, std::size_t{30}}) {
    const Str pre(len, Ch('p'));
    Str s = pre;
    s.append_range(src);
    CHECK(s == pre + exp);
    s = pre;
    s.assign_range(std::span<const S>(src));
    CHECK(s == exp);
    for (std::size_t pos = 0; pos <= len; pos += (len > 5 ? 7 : 1)) {
      s = pre;
      s.insert_range(s.begin() + static_cast<std::ptrdiff_t>(pos), src);
      CHECK(s == pre.substr(0, pos) + exp + pre.substr(pos));
      s = pre;
      s.replace_with_range(s.begin() + static_cast<std::ptrdiff_t>(pos), s.end(), src);
      CHECK(s == pre.substr(0, pos) + exp);
    }
  }
}

int main() {
  run_all<Vec>();
  run_all<Deq>();
  run_all<Lst>();
  run_all<IV>();
  run_string<char>(std::vector<int>{65, 0x141, -1, 0x80, 97});
  run_string<char32_t>(std::vector<int>{65, 0x1F600, -1, 0x80});
  run_string<wchar_t>(std::vector<char>{'a', '\x80', '\xff'});
  run_string<char>(std::vector<signed char>{65, -128, -1});
  run_string<char8_t>(std::vector<char>{'a', '\x80', '\xff'});
  return 0;
}
