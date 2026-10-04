// Exhaustive matrix of the members that take a value which is an element of the same
// container: for every size up to 7, every insertion position p and every source element
// a[k], with exact and with spare capacity. [sequence.reqmts]/24-27 a.insert(p, t): "Inserts a
// copy of t before p"; /32-35 a.insert(p, n, t): "Inserts n copies of t before p"; /22 (Note 1)
// emplace: "args can directly or indirectly refer to a value in a"; push_back/push_front/
// emplace_back/emplace_front append or prepend a copy of t; [vector.capacity]/[deque.capacity]
// resize(sz, c) appends copies of c. Unlike assign(n, t) (/67) none of these has a
// precondition that t is not a reference into a, so the value inserted is the value t had
// before the call. Element types: int (trivially copyable), long strings (heap storage), and a
// type whose moved-from state differs from its value (catches moving the source element
// before copying it).
#include <vector>
#include <deque>
#include <list>
#include <string>
#include <inplace_vector>
#include <iterator>
#include <cstddef>
#include "check.hpp"

struct Marked {
  int v;
  Marked(int x) : v(x) {}
  Marked(const Marked&) = default;
  Marked(Marked&& o) noexcept : v(o.v) { o.v = -1; }
  Marked& operator=(const Marked&) = default;
  Marked& operator=(Marked&& o) noexcept {
    v = o.v;
    o.v = -1;
    return *this;
  }
  friend bool operator==(const Marked&, const Marked&) = default;
};

template <class T>
static T value(int i) {
  if constexpr (std::is_same_v<T, std::string>)
    return std::string(40, static_cast<char>('a' + i)) + std::to_string(i);
  else
    return T(100 + i);
}

template <class C>
static C make(std::size_t n, bool spare) {
  using T = typename C::value_type;
  C c;
  if constexpr (requires { c.reserve(0u); }) {
    if (spare) c.reserve(n + 12);
  }
  for (std::size_t i = 0; i < n; ++i) c.push_back(value<T>(static_cast<int>(i)));
  if constexpr (requires { c.shrink_to_fit(); }) {
    if (!spare) c.shrink_to_fit();
  }
  return c;
}

template <class C>
static std::vector<typename C::value_type> model(std::size_t n) {
  std::vector<typename C::value_type> m;
  for (std::size_t i = 0; i < n; ++i) m.push_back(value<typename C::value_type>(static_cast<int>(i)));
  return m;
}

template <class C, class M>
static bool equal(const C& c, const M& m) {
  if (static_cast<std::size_t>(std::distance(c.begin(), c.end())) != m.size()) return false;
  auto it = c.begin();
  for (std::size_t i = 0; i < m.size(); ++i, ++it)
    if (!(*it == m[i])) return false;
  return true;
}

template <class C>
static auto at(C& c, std::size_t k) -> typename C::value_type& {
  auto it = c.begin();
  std::advance(it, static_cast<std::ptrdiff_t>(k));
  return *it;
}

template <class C>
static void run() {
  using T = typename C::value_type;
  for (std::size_t n = 1; n <= 7; ++n) {
    for (int spare = 0; spare < 2; ++spare) {
      for (std::size_t k = 0; k < n; ++k) {
        const T v = value<T>(static_cast<int>(k));
        for (std::size_t pos = 0; pos <= n; ++pos) {
          auto mpos = [&](std::vector<T>& m) { return m.begin() + static_cast<std::ptrdiff_t>(pos); };
          {
            C c = make<C>(n, spare);
            auto p = c.begin();
            std::advance(p, static_cast<std::ptrdiff_t>(pos));
            c.insert(p, at(c, k));
            auto m = model<C>(n);
            m.insert(mpos(m), v);
            CHECK(equal(c, m));
          }
          {
            C c = make<C>(n, spare);
            auto p = c.begin();
            std::advance(p, static_cast<std::ptrdiff_t>(pos));
            c.emplace(p, at(c, k));
            auto m = model<C>(n);
            m.insert(mpos(m), v);
            CHECK(equal(c, m));
          }
          for (std::size_t cnt = 0; cnt <= 4; ++cnt) {
            C c = make<C>(n, spare);
            auto p = c.begin();
            std::advance(p, static_cast<std::ptrdiff_t>(pos));
            c.insert(p, cnt, at(c, k));
            auto m = model<C>(n);
            m.insert(mpos(m), cnt, v);
            CHECK(equal(c, m));
          }
        }
        {
          C c = make<C>(n, spare);
          c.push_back(at(c, k));
          c.emplace_back(at(c, k));
          auto m = model<C>(n);
          m.push_back(v);
          m.push_back(v);
          CHECK(equal(c, m));
        }
        if constexpr (requires(C c) { c.push_front(v); }) {
          C c = make<C>(n, spare);
          c.push_front(at(c, k));
          c.emplace_front(at(c, k + 1));  // the old a[k] is now a[k + 1]
          auto m = model<C>(n);
          m.insert(m.begin(), v);
          m.insert(m.begin(), v);
          CHECK(equal(c, m));
        }
        if constexpr (requires(C c) { c.resize(1u, v); }) {
          for (std::size_t sz = n; sz <= n + 9; sz += 3) {
            C c = make<C>(n, spare);
            c.resize(sz, at(c, k));
            auto m = model<C>(n);
            m.resize(sz, v);
            CHECK(equal(c, m));
          }
        }
      }
    }
  }
}

template <template <class> class C>
static void run_types() {
  run<C<int>>();
  run<C<std::string>>();
  run<C<Marked>>();
}

template <class T>
using Vec = std::vector<T>;
template <class T>
using Deq = std::deque<T>;
template <class T>
using Lst = std::list<T>;
template <class T>
using IV = std::inplace_vector<T, 32>;

// basic_string: insert(p, c), insert(p, n, c), insert(pos, n, c), push_back(c), append(n, c),
// resize(n, c), replace(pos, len, n, c) with c an element of the same string.
static void run_string() {
  for (std::size_t n = 1; n <= 40; n += (n < 16 ? 1 : 7)) {
    std::string base;
    for (std::size_t i = 0; i < n; ++i) base.push_back(static_cast<char>('a' + i % 26));
    for (std::size_t k = 0; k < n; k += (n > 10 ? 3 : 1)) {
      const char v = base[k];
      for (std::size_t pos = 0; pos <= n; ++pos) {
        std::string s = base;
        s.insert(s.begin() + static_cast<std::ptrdiff_t>(pos), s[k]);
        CHECK(s == base.substr(0, pos) + v + base.substr(pos));
        for (std::size_t cnt : {std::size_t{1}, std::size_t{3}, std::size_t{30}}) {
          s = base;
          s.insert(s.begin() + static_cast<std::ptrdiff_t>(pos), cnt, s[k]);
          CHECK(s == base.substr(0, pos) + std::string(cnt, v) + base.substr(pos));
          s = base;
          s.insert(pos, cnt, s[k]);
          CHECK(s == base.substr(0, pos) + std::string(cnt, v) + base.substr(pos));
          s = base;
          s.replace(pos, 1, cnt, s[k]);
          CHECK(s == base.substr(0, pos) + std::string(cnt, v) + (pos < n ? base.substr(pos + 1) : ""));
        }
      }
      std::string s = base;
      s.push_back(s[k]);
      s.append(20, s[k]);
      s.resize(s.size() + 30, s[k]);
      CHECK(s == base + std::string(51, v));
    }
  }
}

int main() {
  run_types<Vec>();
  run_types<Deq>();
  run_types<Lst>();
  run_types<IV>();
  run_string();
  return 0;
}
