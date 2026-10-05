// Long pseudo-random sequences of modifiers on vector, deque, list, inplace_vector and
// basic_string, compared after every step against a plain array model. The element type of the
// non-string containers counts its live objects, so every step also checks that exactly size()
// elements exist (nothing leaked or destroyed twice).
// [sequence.reqmts]: a.insert(p, t) / a.emplace(p, args) return an iterator to the new element;
// a.insert(p, n, t), a.insert(p, i, j), a.insert_range(p, rg), a.insert(p, il) return an
// iterator to the first inserted element, or p if nothing is inserted; a.erase(q) and
// a.erase(q1, q2) return the iterator following the erased elements; a.assign(n, t),
// a.assign(i, j), a.assign_range(rg); a.append_range(rg), a.prepend_range(rg) (deque, list);
// push/pop/emplace at the ends; resize(n) value-initializes and resize(n, t) copies;
// [vector.erasure] etc.: erase(c, v) / erase_if(c, pred) return the number removed. Input-only
// and forward ranges both feed the range members. swap exchanges contents.
#include <deque>
#include <inplace_vector>
#include <list>
#include <string>
#include <vector>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include "check.hpp"

long live = 0;
struct Tracked {
  int* p;
  Tracked() : p(new int(0)) { ++live; }
  Tracked(int v) : p(new int(v)) { ++live; }
  Tracked(const Tracked& o) : p(new int(*o.p)) { ++live; }
  Tracked(Tracked&& o) noexcept : p(new int(*o.p)) { ++live; }
  Tracked& operator=(const Tracked& o) {
    *p = *o.p;
    return *this;
  }
  Tracked& operator=(Tracked&& o) noexcept {
    *p = *o.p;
    return *this;
  }
  ~Tracked() {
    delete p;
    --live;
  }
  int value() const { return *p; }
  friend bool operator==(const Tracked& a, const Tracked& b) { return *a.p == *b.p; }
};
int val(const Tracked& t) { return t.value(); }
int val(char c) { return c; }
int val(int x) { return x; }

// Single-pass input view over ints (for the range members).
struct InputInts {
  const int* b;
  const int* e;
  struct iterator {
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    const int* p;
    int operator*() const { return *p; }
    iterator& operator++() {
      ++p;
      return *this;
    }
    void operator++(int) { ++p; }
    friend bool operator==(const iterator& a, const iterator& b) { return a.p == b.p; }
  };
  iterator begin() const { return {b}; }
  iterator end() const { return {e}; }
};
static_assert(std::ranges::input_range<InputInts> && !std::ranges::forward_range<InputInts>);

unsigned st = 2026u;
unsigned rnd(unsigned n) {
  st ^= st << 13;
  st ^= st >> 17;
  st ^= st << 5;
  return st % n;
}

struct Model {
  int v[4000];
  int n = 0;
  void insert(int pos, int count, const int* src) {
    for (int k = n - 1; k >= pos; --k) v[k + count] = v[k];
    for (int k = 0; k < count; ++k) v[pos + k] = src[k];
    n += count;
  }
  void erase(int pos, int count) {
    for (int k = pos; k + count < n; ++k) v[k] = v[k + count];
    n -= count;
  }
};

template <class C>
constexpr bool has_front_ops = requires(C c) { c.push_front(c.front()); };
template <class C>
constexpr bool is_string = requires(C c) { c.c_str(); };

template <class C>
typename C::value_type make(int x) {
  if constexpr (is_string<C>) return static_cast<char>(x);  // x is already norm<C>'ed
  else return typename C::value_type(x);
}
template <class C>
int norm(int x) {
  if constexpr (is_string<C>) return 'a' + (x % 26 + 26) % 26;
  else return x;
}

template <class C>
bool same(const C& c, const Model& m) {
  if (static_cast<int>(c.size()) != m.n) return false;
  int i = 0;
  for (auto& x : c)
    if (val(x) != m.v[i++]) return false;
  return true;
}

template <class C>
void run(int steps, int max_size) {
  C c;
  Model m;
  int next = 1;
  auto pos_it = [&](int pos) { return std::next(c.begin(), pos); };
  for (int step = 0; step < steps; ++step) {
    const int pos = static_cast<int>(rnd(static_cast<unsigned>(m.n + 1)));
    const bool room = m.n + 8 <= max_size;
    int src[8];
    const int cnt = static_cast<int>(rnd(6));
    for (int k = 0; k < cnt; ++k) src[k] = norm<C>(next++);
    switch (rnd(16)) {
      case 0:
        if (!room) break;
        {
          auto r = c.insert(pos_it(pos), make<C>(src[0] = norm<C>(next++)));
          CHECK(r == pos_it(pos));
          m.insert(pos, 1, src);
        }
        break;
      case 1:
        if (!room) break;
        {
          src[0] = norm<C>(next++);
          typename C::iterator r;
          if constexpr (requires { c.emplace(c.begin(), make<C>(0)); }) r = c.emplace(pos_it(pos), make<C>(src[0]));
          else r = c.insert(pos_it(pos), make<C>(src[0]));  // basic_string has no emplace
          CHECK(r == pos_it(pos));
          m.insert(pos, 1, src);
        }
        break;
      case 2:
        if (!room) break;
        {
          int x = norm<C>(next++);
          for (int k = 0; k < cnt; ++k) src[k] = x;
          auto r = c.insert(pos_it(pos), static_cast<typename C::size_type>(cnt), make<C>(x));
          CHECK(r == pos_it(pos));
          m.insert(pos, cnt, src);
        }
        break;
      case 3:
        if (!room) break;
        {
          std::list<typename C::value_type> tmp;
          for (int k = 0; k < cnt; ++k) tmp.push_back(make<C>(src[k]));
          auto r = c.insert(pos_it(pos), tmp.begin(), tmp.end());
          CHECK(r == pos_it(pos));
          m.insert(pos, cnt, src);
        }
        break;
      case 4:
        if (!room) break;
        {
          auto r = c.insert_range(pos_it(pos), InputInts{src, src + cnt} | std::views::transform([](int x) { return make<C>(x); }));
          CHECK(r == pos_it(pos));
          m.insert(pos, cnt, src);
        }
        break;
      case 5:
        if (!room) break;
        {
          std::vector<typename C::value_type> tmp;
          for (int k = 0; k < cnt; ++k) tmp.push_back(make<C>(src[k]));
          if (rnd(2)) {
            c.append_range(tmp);
            m.insert(m.n, cnt, src);
          } else if constexpr (requires { c.prepend_range(tmp); }) {
            c.prepend_range(tmp);
            m.insert(0, cnt, src);
          } else {
            auto r = c.insert_range(pos_it(pos), tmp);
            CHECK(r == pos_it(pos));
            m.insert(pos, cnt, src);
          }
        }
        break;
      case 6:
      case 7:
        if (m.n == 0) break;
        {
          int p = static_cast<int>(rnd(static_cast<unsigned>(m.n)));
          auto r = c.erase(pos_it(p));
          CHECK(r == pos_it(p));
          m.erase(p, 1);
        }
        break;
      case 8: {
        int len = static_cast<int>(rnd(static_cast<unsigned>(m.n - pos + 1)));
        if (len > 5) len = 5;
        auto r = c.erase(pos_it(pos), pos_it(pos + len));
        CHECK(r == pos_it(pos));
        m.erase(pos, len);
        break;
      }
      case 9:
        if (!room) break;
        src[0] = norm<C>(next++);
        if constexpr (requires { c.emplace_back(make<C>(0)); }) {
          if (rnd(2)) c.emplace_back(make<C>(src[0]));
          else c.push_back(make<C>(src[0]));
        } else {
          c.push_back(make<C>(src[0]));
        }
        m.insert(m.n, 1, src);
        if constexpr (has_front_ops<C>) {
          src[0] = norm<C>(next++);
          if (rnd(2)) c.push_front(make<C>(src[0]));
          else c.emplace_front(make<C>(src[0]));
          m.insert(0, 1, src);
        }
        break;
      case 10:
        if (m.n == 0) break;
        c.pop_back();
        m.erase(m.n - 1, 1);
        if constexpr (has_front_ops<C>) {
          if (m.n > 0) {
            c.pop_front();
            m.erase(0, 1);
          }
        }
        break;
      case 11: {  // resize, value-initializing or copying
        int to = static_cast<int>(rnd(static_cast<unsigned>(m.n + (room ? 8 : 1))));
        if (rnd(2)) {
          c.resize(static_cast<typename C::size_type>(to));
          while (m.n > to) m.erase(m.n - 1, 1);
          int zero = norm<C>(0);
          if constexpr (is_string<C>) zero = 0;
          while (m.n < to) m.insert(m.n, 1, &zero);
        } else {
          int x = norm<C>(next++);
          c.resize(static_cast<typename C::size_type>(to), make<C>(x));
          while (m.n > to) m.erase(m.n - 1, 1);
          while (m.n < to) m.insert(m.n, 1, &x);
        }
        break;
      }
      case 12: {  // assign forms
        if (rnd(8) != 0) break;
        int n2 = static_cast<int>(rnd(static_cast<unsigned>(max_size < 40 ? max_size : 40)));
        int buf[40];
        for (int k = 0; k < n2; ++k) buf[k] = norm<C>(next++);
        switch (rnd(3)) {
          case 0: {
            std::vector<typename C::value_type> tmp;
            for (int k = 0; k < n2; ++k) tmp.push_back(make<C>(buf[k]));
            c.assign(tmp.begin(), tmp.end());
            break;
          }
          case 1: c.assign_range(InputInts{buf, buf + n2} | std::views::transform([](int x) { return make<C>(x); })); break;
          default: {
            for (int k = 1; k < n2; ++k) buf[k] = buf[0];
            c.assign(static_cast<typename C::size_type>(n2), make<C>(n2 ? buf[0] : 0));
          }
        }
        m.n = 0;
        m.insert(0, n2, buf);
        break;
      }
      case 13: {  // erase / erase_if
        int mod = 2 + static_cast<int>(rnd(4));
        int removed = 0;
        for (int k = 0; k < m.n;) {
          if (m.v[k] % mod == 0) {
            m.erase(k, 1);
            ++removed;
          } else {
            ++k;
          }
        }
        auto r = std::erase_if(c, [&](const auto& x) { return val(x) % mod == 0; });
        CHECK(static_cast<int>(r) == removed);
        if (m.n > 0) {
          int target = m.v[rnd(static_cast<unsigned>(m.n))];
          int again = 0;
          for (int k = 0; k < m.n;) {
            if (m.v[k] == target) {
              m.erase(k, 1);
              ++again;
            } else {
              ++k;
            }
          }
          CHECK(static_cast<int>(std::erase(c, make<C>(target))) == again);
        }
        break;
      }
      case 14: {  // swap with a copy, both ways
        C other(c);
        C empty;
        c.swap(empty);
        CHECK(c.empty() && same(empty, m));
        using std::swap;
        swap(c, other);
        CHECK(same(c, m) && other.empty());
        break;
      }
      default: {
        if constexpr (requires { c.shrink_to_fit(); }) c.shrink_to_fit();
        if constexpr (requires { c.capacity(); }) CHECK(c.capacity() >= c.size());
        if (rnd(40) == 0) {
          c.clear();
          m.n = 0;
        }
      }
    }
    CHECK(same(c, m));
    if constexpr (std::is_same_v<typename C::value_type, Tracked>) CHECK(live == static_cast<long>(c.size()));
  }
}

int main() {
  run<std::vector<Tracked>>(6000, 3000);
  run<std::deque<Tracked>>(6000, 3000);
  run<std::list<Tracked>>(4000, 3000);
  run<std::inplace_vector<Tracked, 60>>(6000, 60);
  run<std::string>(6000, 3000);
  run<std::vector<int>>(6000, 3000);
  CHECK(live == 0);
}
