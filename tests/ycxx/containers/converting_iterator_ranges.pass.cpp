// Iterator-pair members of the sequence containers whose source value type differs from T.
// [sequence.reqmts]: X(i, j) "Constructs a sequence container equal to the range [i, j). Each
// iterator in the range [i, j) is dereferenced exactly once" (/11-/14; T is
// Cpp17EmplaceConstructible from *i); a.insert(p, i, j) "Inserts copies of elements in [i, j)
// before p" (/40-/43); a.assign(i, j) "Replaces elements in a with a copy of [i, j)" (/60-/63);
// [string.cons]/[string.append]/[string.insert]/[string.replace]: the iterator forms construct a
// basic_string(first, last) of the converted characters. Each element is therefore the value
// T(*it), never the source's object representation: same-size trivially copyable pairs (int to
// float, int to a wrapper whose constructor transforms) and narrowing/widening pairs are
// checked for pointers and input, forward and random access iterators, at the front, middle
// and back, with and without spare capacity.
#include <vector>
#include <deque>
#include <list>
#include <string>
#include <inplace_vector>
#include <iterator>
#include <cstddef>
#include "test_iterators.hpp"
#include "check.hpp"

struct Twice {  // trivially copyable, same size as int, but not a copy of the int
  int v;
  Twice(int x) : v(2 * x + 1) {}
  friend bool operator==(Twice, Twice) = default;
};
static_assert(sizeof(Twice) == sizeof(int));

template <class T, class S>
static std::vector<T> converted(const S* b, const S* e) {
  std::vector<T> r;
  for (; b != e; ++b) r.push_back(static_cast<T>(*b));
  return r;
}

template <class C, class Exp>
static bool equal(const C& c, const Exp& e) {
  if (c.size() != e.size()) return false;
  auto it = c.begin();
  for (std::size_t i = 0; i < e.size(); ++i, ++it)
    if (!(*it == e[i])) return false;
  return true;
}

template <class C, template <class> class It, class S>
static void run_kind(S* src, std::size_t n) {
  using T = typename C::value_type;
  auto mk = [&](std::size_t a) { return It<S>(src + a); };
  const std::vector<T> all = converted<T>(src, src + n);
  // X(i, j)
  {
    C c(mk(0), mk(n));
    CHECK(equal(c, all));
  }
  // assign(i, j): onto a longer, a shorter and an empty container
  for (std::size_t init : {std::size_t{0}, std::size_t{2}, n + 3}) {
    C c(init, static_cast<T>(S{}));
    c.assign(mk(0), mk(n));
    CHECK(equal(c, all));
  }
  // insert(p, i, j) at every position, with and without spare capacity
  const std::vector<T> base = converted<T>(src + n / 2, src + n);
  for (int spare = 0; spare < 2; ++spare) {
    for (std::size_t pos = 0; pos <= base.size(); ++pos) {
      C c(mk(n / 2), mk(n));
      if constexpr (requires { c.reserve(0u); c.capacity(); }) {
        if (spare) c.reserve(c.size() + n + 4);
        else c.shrink_to_fit();
      }
      auto p = c.begin();
      std::advance(p, static_cast<std::ptrdiff_t>(pos));
      auto r = c.insert(p, mk(0), mk(n));
      CHECK(static_cast<std::size_t>(std::distance(c.begin(), r)) == pos);
      std::vector<T> exp(base.begin(), base.begin() + static_cast<std::ptrdiff_t>(pos));
      exp.insert(exp.end(), all.begin(), all.end());
      exp.insert(exp.end(), base.begin() + static_cast<std::ptrdiff_t>(pos), base.end());
      CHECK(equal(c, exp));
    }
  }
}

template <class U>
using Ptr = U*;

template <class C, class S, std::size_t N>
static void run(S (&src)[N]) {
  run_kind<C, Ptr>(src, N);
  run_kind<C, InputIter>(src, N);
  run_kind<C, ForwardIter>(src, N);
  run_kind<C, RandomIter>(src, N);
  // empty source range
  C c(src, src);
  CHECK(c.empty());
}

template <template <class> class C>
static void run_all() {
  int ints[] = {0, 1, -1, 16777217, 2147483647, -2147483647 - 1, 300, 5, 7, 255, 256, 99};
  double dbls[] = {0.5, -1.75, 3.99, 1e9, -2e9, 42.0, 7.25};
  unsigned char uchars[] = {0, 1, 127, 128, 200, 255, 10};
  signed char schars[] = {0, 1, 127, -128, -1, -56, 10};
  bool bools[] = {true, false, true, true};
  run<C<float>>(ints);
  run<C<long long>>(ints);
  run<C<char>>(ints);
  run<C<Twice>>(ints);
  run<C<int>>(dbls);
  run<C<float>>(dbls);
  run<C<int>>(uchars);
  run<C<int>>(schars);
  run<C<unsigned>>(schars);
  run<C<int>>(bools);
  run<C<double>>(ints);
}

template <class T>
using Vec = std::vector<T>;
template <class T>
using Deq = std::deque<T>;
template <class T>
using Lst = std::list<T>;
template <class T>
using IV = std::inplace_vector<T, 64>;

template <class Ch, class S, std::size_t N>
static void run_string(S (&src)[N]) {
  using Str = std::basic_string<Ch>;
  Str exp;
  for (S s : src) exp.push_back(static_cast<Ch>(s));
  CHECK(Str(src, src + N) == exp);
  CHECK(Str(InputIter<S>(src), InputIter<S>(src + N)) == exp);
  for (std::size_t len : {std::size_t{0}, std::size_t{3}, std::size_t{40}}) {
    const Str pre(len, Ch('p'));
    Str s = pre;
    s.append(ForwardIter<S>(src), ForwardIter<S>(src + N));
    CHECK(s == pre + exp);
    s = pre;
    s.append(InputIter<S>(src), InputIter<S>(src + N));
    CHECK(s == pre + exp);
    s = pre;
    s.assign(src, src + N);
    CHECK(s == exp);
    for (std::size_t pos = 0; pos <= len; pos += (len > 5 ? 7 : 1)) {
      s = pre;
      s.insert(s.begin() + static_cast<std::ptrdiff_t>(pos), InputIter<S>(src), InputIter<S>(src + N));
      CHECK(s == pre.substr(0, pos) + exp + pre.substr(pos));
      s = pre;
      s.insert(s.begin() + static_cast<std::ptrdiff_t>(pos), src, src + N);
      CHECK(s == pre.substr(0, pos) + exp + pre.substr(pos));
      s = pre;
      s.replace(s.begin() + static_cast<std::ptrdiff_t>(pos), s.end(), ForwardIter<S>(src), ForwardIter<S>(src + N));
      CHECK(s == pre.substr(0, pos) + exp);
    }
  }
}

int main() {
  run_all<Vec>();
  run_all<Deq>();
  run_all<Lst>();
  run_all<IV>();

  int ints[] = {65, 300, -1, 0x1F600, 97, 0x80, 0xFF, 0x100};
  unsigned char uchars[] = {0x41, 0x80, 0xFF, 0x7F, 0};
  char chars[] = {'a', '\x80', '\xff', 'z'};
  run_string<char>(ints);
  run_string<char>(uchars);
  run_string<wchar_t>(chars);
  run_string<wchar_t>(uchars);
  run_string<char32_t>(ints);
  run_string<char16_t>(ints);
  run_string<char8_t>(chars);
  return 0;
}
