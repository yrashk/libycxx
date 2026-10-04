// [deque.modifiers], [sequence.reqmts]/24-55: a long pseudo-random sequence of insert (single,
// n copies, range, initializer list), emplace, erase (single and range), push/pop at both
// ends, resize and clear on a deque, compared after every step against a plain array model.
// Positions cover the front half, the back half, both ends and the middle, so both
// directions of element shifting and block growth at either end are exercised. Each insert
// returns an iterator to the first inserted element (or p when nothing is inserted) and each
// erase returns an iterator to the element following the erased ones.
#include <deque>
#include <cstddef>
#include <initializer_list>
#include "container_values.hpp"
#include "check.hpp"

template <class T>
struct Model {
  int v[6000];
  int n = 0;
  constexpr void insert(int pos, int count, const int* src) {
    for (int k = n - 1; k >= pos; --k) v[k + count] = v[k];
    for (int k = 0; k < count; ++k) v[pos + k] = src[k];
    n += count;
  }
  constexpr void erase(int pos, int count) {
    for (int k = pos; k + count < n; ++k) v[k] = v[k + count];
    n -= count;
  }
};

template <class T>
constexpr bool same(const std::deque<T>& d, const Model<T>& m) {
  if (d.size() != static_cast<std::size_t>(m.n)) return false;
  int k = 0;
  for (const auto& x : d)
    if (!(x == val<T>(m.v[k++]))) return false;
  return true;
}

template <class T>
constexpr bool run(int steps) {
  std::deque<T> d;
  Model<T>* mp = new Model<T>;
  Model<T>& m = *mp;
  unsigned seed = 12345u;
  auto rnd = [&](int bound) {
    seed = seed * 1103515245u + 12345u;
    return bound <= 0 ? 0 : static_cast<int>((seed >> 8) % static_cast<unsigned>(bound));
  };
  bool ok = true;
  for (int s = 0; s < steps && ok; ++s) {
    int op = rnd(12);
    int pos = rnd(m.n + 1);
    int value = rnd(80);
    auto p = d.cbegin() + pos;
    if (m.n > 3000 && op < 7) op = 8;  // keep the size bounded
    switch (op) {
      case 0: {
        auto it = d.insert(p, val<T>(value));
        m.insert(pos, 1, &value);
        ok = it - d.begin() == pos;
        break;
      }
      case 1: {
        int c = rnd(40);
        int src[40];
        for (int k = 0; k < c; ++k) src[k] = value;
        auto it = d.insert(p, static_cast<std::size_t>(c), val<T>(value));
        m.insert(pos, c, src);
        ok = it - d.begin() == pos;
        break;
      }
      case 2: {
        int c = rnd(30);
        T arr[30];
        int src[30];
        for (int k = 0; k < c; ++k) {
          src[k] = (value + k) % 80;
          arr[k] = val<T>(src[k]);
        }
        auto it = d.insert(p, arr, arr + c);
        m.insert(pos, c, src);
        ok = it - d.begin() == pos;
        break;
      }
      case 3: {
        int src[3] = {value, (value + 1) % 80, (value + 2) % 80};
        auto it = d.insert(p, {val<T>(src[0]), val<T>(src[1]), val<T>(src[2])});
        m.insert(pos, 3, src);
        ok = it - d.begin() == pos;
        break;
      }
      case 4: {
        auto it = d.emplace(p, val<T>(value));
        m.insert(pos, 1, &value);
        ok = it - d.begin() == pos && *it == val<T>(value);
        break;
      }
      case 5:
        d.push_front(val<T>(value));
        m.insert(0, 1, &value);
        break;
      case 6:
        d.push_back(val<T>(value));
        m.insert(m.n, 1, &value);
        break;
      case 7:
        if (m.n > 0) {
          if (pos == m.n) --pos;
          auto it = d.erase(d.cbegin() + pos);
          m.erase(pos, 1);
          ok = it - d.begin() == pos;
        }
        break;
      case 8: {
        int c = rnd(m.n - pos + 1);
        auto it = d.erase(d.cbegin() + pos, d.cbegin() + pos + c);
        m.erase(pos, c);
        ok = it - d.begin() == pos;
        break;
      }
      case 9:
        if (m.n > 0) {
          if (value % 2) { d.pop_front(); m.erase(0, 1); }
          else { d.pop_back(); m.erase(m.n - 1, 1); }
        }
        break;
      case 10: {
        int sz = rnd(m.n + 20);
        if (sz > m.n) {
          int src[20];
          for (int k = 0; k < 20; ++k) src[k] = value;
          d.resize(static_cast<std::size_t>(sz), val<T>(value));
          m.insert(m.n, sz - m.n, src);
        } else {
          d.resize(static_cast<std::size_t>(sz));
          m.erase(sz, m.n - sz);
        }
        break;
      }
      case 11:
        if (rnd(50) == 0) {
          d.clear();
          m.n = 0;
        }
        break;
    }
    ok = ok && same(d, m);
  }
  delete mp;
  return ok;
}

static_assert(run<int>(150));
static_assert(run<Elem>(60));

int main() {
  CHECK(run<int>(20000));
  CHECK(run<Elem>(5000));
  return 0;
}
