// Every form of insertion and erasure at either end of a deque, at sizes and offsets that cross
// internal block boundaries, with the validity guarantees of [deque.modifiers]:
// /1 "An insertion at either end of the deque invalidates all the iterators to the deque, but
// has no effect on the validity of references to elements of the deque" -- for insert(begin()
// or end(), x / n, x / i, j / il), emplace at either end, emplace_front/back, push_*,
// prepend_range/append_range, and resize growing ([deque.capacity]: "appends sz - size()
// elements");
// /4 "An erase operation that erases the last element of a deque invalidates only the
// past-the-end iterator and all iterators and references to the erased elements. An erase
// operation that erases the first element of a deque but not the last element invalidates
// only iterators and references to the erased elements" -- for erase(q), erase(q1, q2) at
// either end and resize shrinking ([deque.capacity]: "erases the last size() - sz elements").
// Return values: insert returns an iterator to the first inserted element (or pos when nothing
// is inserted), erase "an iterator pointing to the element pointed to by q2 prior to any
// elements being erased", emplace_front/back a reference to the new element.
#include <deque>
#include <initializer_list>
#include <iterator>
#include <vector>
#include "check.hpp"

struct E {
  long v;
  char pad[40];
  E(long x = -7) : v(x), pad{} {}
};

static std::vector<long> model;
static long next_v = 1;

static void check_all(const std::deque<E>& d) {
  CHECK(d.size() == model.size());
  CHECK(d.end() - d.begin() == static_cast<long>(model.size()));
  for (std::size_t i = 0; i < model.size(); ++i) CHECK(d[i].v == model[i]);
  CHECK(static_cast<std::size_t>(std::distance(d.begin(), d.end())) == model.size());
}

// pointers to every element (references), checked after an operation that keeps them
struct Refs {
  std::vector<const E*> p;
  std::vector<long> v;
  explicit Refs(const std::deque<E>& d) {
    for (const E& e : d) {
      p.push_back(&e);
      v.push_back(e.v);
    }
  }
  void check_kept(std::size_t from, std::size_t to) const {
    for (std::size_t i = from; i < to; ++i) CHECK(p[i]->v == v[i]);
  }
};

static std::deque<E> fresh(int n, int rotate) {
  std::deque<E> d;
  model.clear();
  for (int i = 0; i < rotate; ++i) {  // move the start through the block structure
    d.push_back(E(0));
    d.pop_front();
  }
  for (int i = 0; i < n; ++i) {
    d.push_back(E(next_v));
    model.push_back(next_v++);
  }
  return d;
}

static void insertions(int n, int rotate) {
  for (int form = 0; form < 14; ++form) {
    for (int k : {0, 1, 2, 7, 33, 100}) {
      std::deque<E> d = fresh(n, rotate);
      Refs refs(d);
      const bool front = form % 2 == 0;
      std::vector<long> add;
      for (int i = 0; i < k; ++i) add.push_back(next_v++);
      std::vector<E> src(add.begin(), add.end());
      std::deque<E>::iterator it;
      bool returns_it = true;
      switch (form / 2) {
        case 0: it = d.insert(front ? d.begin() : d.end(), src.begin(), src.end()); break;
        case 1: {
          it = d.insert(front ? d.cbegin() : d.cend(), static_cast<std::size_t>(k), E(add.empty() ? 0 : add[0]));
          for (long& x : add) x = add.empty() ? 0 : add[0];
          break;
        }
        case 2:
          if (k > 3) continue;
          if (k == 0) it = d.insert(front ? d.begin() : d.end(), std::initializer_list<E>{});
          else if (k == 1) it = d.insert(front ? d.begin() : d.end(), {E(add[0])});
          else if (k == 2) it = d.insert(front ? d.begin() : d.end(), {E(add[0]), E(add[1])});
          else continue;
          break;
        case 3:
          if (k != 1) continue;
          it = d.emplace(front ? d.begin() : d.end(), add[0]);
          break;
        case 4:
          if (k != 1) continue;
          {
            E& r = front ? d.emplace_front(add[0]) : d.emplace_back(add[0]);
            CHECK(&r == (front ? &d.front() : &d.back()));
          }
          returns_it = false;
          break;
        case 5:
          if (front) d.prepend_range(src);
          else d.append_range(src);
          returns_it = false;
          break;
        case 6:
          if (front) {
            // insert(begin(), x) one at a time in reverse keeps the order of add
            for (int i = k - 1; i >= 0; --i) it = d.insert(d.begin(), E(add[static_cast<std::size_t>(i)]));
            if (k == 0) it = d.begin();
          } else {
            d.resize(d.size() + static_cast<std::size_t>(k));  // value-initialized E(): v == -7
            for (long& x : add) x = -7;
            returns_it = false;
          }
          break;
      }
      if (front) model.insert(model.begin(), add.begin(), add.end());
      else model.insert(model.end(), add.begin(), add.end());
      check_all(d);
      refs.check_kept(0, refs.p.size());  // references survive every end insertion
      for (std::size_t i = 0; i < refs.p.size(); ++i) CHECK(refs.p[i] == &d[front ? i + add.size() : i]);
      if (returns_it) CHECK(it == (front ? d.begin() : d.begin() + n));
    }
  }
}

static void erasures(int n, int rotate) {
  for (int k = 1; k <= n; k += (k < 4 ? 1 : 13)) {
    // at the front, not the last element: end() and the remaining elements stay valid
    for (int form = 0; form < 4; ++form) {
      std::deque<E> d = fresh(n, rotate);
      Refs refs(d);
      auto e = d.end();
      std::vector<std::deque<E>::iterator> its;
      for (int i = 0; i < n; ++i) its.push_back(d.begin() + i);
      std::deque<E>::iterator r;
      const bool all = k == n;
      switch (form) {
        case 0: r = d.erase(d.begin(), d.begin() + k); break;
        case 1:
          for (int i = 0; i < k; ++i) r = d.erase(d.begin());
          break;
        case 2: r = d.erase(d.cbegin(), d.cbegin() + k); break;
        case 3:
          for (int i = 0; i < k; ++i) d.pop_front();
          r = d.begin();
          break;
      }
      model.erase(model.begin(), model.begin() + k);
      check_all(d);
      CHECK(r == d.begin());
      if (!all) {
        CHECK(e == d.end());
        for (int i = k; i < n; ++i) CHECK(its[static_cast<std::size_t>(i)] == d.begin() + (i - k));
        refs.check_kept(static_cast<std::size_t>(k), static_cast<std::size_t>(n));
      }
    }
    // at the back: begin() and the remaining elements stay valid
    for (int form = 0; form < 4; ++form) {
      std::deque<E> d = fresh(n, rotate);
      Refs refs(d);
      auto b = d.begin();
      std::vector<std::deque<E>::iterator> its;
      for (int i = 0; i < n; ++i) its.push_back(d.begin() + i);
      std::deque<E>::iterator r = d.end();
      switch (form) {
        case 0: r = d.erase(d.end() - k, d.end()); break;
        case 1:
          for (int i = 0; i < k; ++i) r = d.erase(d.end() - 1);
          break;
        case 2: d.resize(static_cast<std::size_t>(n - k)); r = d.end(); break;
        case 3:
          for (int i = 0; i < k; ++i) d.pop_back();
          r = d.end();
          break;
      }
      model.resize(static_cast<std::size_t>(n - k));
      check_all(d);
      CHECK(r == d.end());
      if (k < n) {
        CHECK(b == d.begin());
        for (int i = 0; i < n - k; ++i) CHECK(its[static_cast<std::size_t>(i)] == d.begin() + i);
        refs.check_kept(0, static_cast<std::size_t>(n - k));
      }
    }
  }
}

int main() {
  for (int n : {0, 1, 2, 5, 31, 64, 65, 100, 200})
    for (int rotate : {0, 1, 13, 50, 63, 64, 101}) {
      insertions(n, rotate);
      if (n > 0) erasures(n, rotate);
    }
}
