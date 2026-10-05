// vector insert/emplace/erase at every position of vectors of trivially copyable element types
// (and of types that are only partly trivial), with and without spare capacity, compared
// against an array model:
// [sequence.reqmts]: a.insert(p, t) "Inserts a copy of t before p" (t may be an element of a:
// no precondition excludes it), a.insert(p, n, t), a.insert(p, i, j), a.insert(p, il),
// a.emplace(p, args) "Inserts an object of type T constructed with ... args before p";
// a.erase(q), a.erase(q1, q2); every insert returns an iterator to the first inserted element
// (or p), every erase "an iterator that points to the element immediately following q prior to
// the element being erased".
// [vector.modifiers]/2: "If no reallocation happens, then references, pointers, and iterators
// before the insertion point remain valid"; /4: erase "Invalidates iterators and references at
// or after the point of the erase"; /6: "The destructor of T is called the number of times
// equal to the number of the elements erased, but the assignment operator of T is called the
// number of times equal to the number of elements in the vector after the erased elements".
// Element types: int, unsigned char, double, a padded aggregate, a 40-byte aggregate, a type
// with trivial copy construction but a user-provided assignment operator, and a type with
// trivial copy operations but a user-provided destructor (neither of the last two is
// trivially copyable: their assignments/destructions are observable and counted). Types with a
// const or reference member (trivially copyable, not assignable) can only grow at the end:
// push_back/emplace_back/reserve/shrink_to_fit/copies, checked separately.
#include <vector>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include "check.hpp"

static long assigns = 0;
static long dtors = 0;

struct Pad3 {
  int a;
  short b;
  char c;
  Pad3(int x) : a(x), b(static_cast<short>(x * 3)), c(static_cast<char>(x)) {}
  int key() const {
    CHECK(b == static_cast<short>(a * 3) && c == static_cast<char>(a));
    return a;
  }
};
struct Big {
  long x[5];
  Big(int v) : x{v, v + 1, v + 2, v + 3, v + 4} {}
  int key() const {
    for (int i = 1; i < 5; ++i) CHECK(x[i] == x[0] + i);
    return static_cast<int>(x[0]);
  }
};
struct AssignCounted {  // trivial copy/move construction, user-provided assignment
  int v;
  AssignCounted(int x) : v(x) {}
  AssignCounted(const AssignCounted&) = default;
  AssignCounted& operator=(const AssignCounted& o) {
    ++assigns;
    v = o.v;
    return *this;
  }
  int key() const { return v; }
};
static_assert(std::is_trivially_copy_constructible_v<AssignCounted>);
static_assert(!std::is_trivially_copyable_v<AssignCounted>);
struct DtorCounted {  // trivial copy operations, user-provided destructor
  int v;
  DtorCounted(int x) : v(x) {}
  DtorCounted(const DtorCounted&) = default;
  DtorCounted& operator=(const DtorCounted&) = default;
  ~DtorCounted() { ++dtors; }
  int key() const { return v; }
};
static_assert(std::is_trivially_copy_assignable_v<DtorCounted>);
static_assert(!std::is_trivially_copyable_v<DtorCounted>);

template <class T>
int key(const T& t) {
  if constexpr (std::is_arithmetic_v<T>) return static_cast<int>(t);
  else return t.key();
}
template <class T>
T make(int v) {
  if constexpr (std::is_same_v<T, unsigned char>) return static_cast<unsigned char>(v % 251);
  else return T(v);
}
template <class T>
int norm(int v) {
  return std::is_same_v<T, unsigned char> ? v % 251 : v;
}

constexpr int MAXN = 9;

template <class T>
std::vector<T> build(int n, bool spare) {
  std::vector<T> v;
  if (spare) v.reserve(static_cast<std::size_t>(n) + 8);
  for (int i = 0; i < n; ++i) v.push_back(make<T>(10 + i));
  if (!spare) {
    v.shrink_to_fit();  // non-binding; capacity()==size() is not relied upon
  }
  return v;
}

template <class T>
void expect(const std::vector<T>& v, const int* m, int n) {
  CHECK(static_cast<int>(v.size()) == n);
  for (int i = 0; i < n; ++i) CHECK(key(v[static_cast<std::size_t>(i)]) == norm<T>(m[i]));
}

// model of 10 + i, with `ins` values inserted at position p
static int model[64];
static int model_n;
static void model_init(int n) {
  model_n = n;
  for (int i = 0; i < n; ++i) model[i] = 10 + i;
}
static void model_insert(int p, std::initializer_list<int> vals) {
  int k = static_cast<int>(vals.size());
  for (int i = model_n - 1; i >= p; --i) model[i + k] = model[i];
  int i = p;
  for (int x : vals) model[i++] = x;
  model_n += k;
}
static void model_erase(int f, int l) {
  for (int i = l; i < model_n; ++i) model[i - (l - f)] = model[i];
  model_n -= l - f;
}

// pointers before p stay valid (and keep their values) when no reallocation happens
template <class T>
struct Before {
  const T* data;
  std::size_t cap;
  int p;
  explicit Before(const std::vector<T>& v, int pos) : data(v.data()), cap(v.capacity()), p(pos) {}
  void check(const std::vector<T>& v) const {
    if (v.data() != data) {
      CHECK(v.capacity() != cap || v.size() > cap);  // only by reallocation
      return;
    }
    for (int i = 0; i < p; ++i) CHECK(key(data[i]) == norm<T>(model[i]));
  }
};

template <class T>
void inserts() {
  for (int n = 0; n <= MAXN; ++n)
    for (int spare = 0; spare < 2; ++spare)
      for (int p = 0; p <= n; ++p) {
        // insert(p, t) with t an element of the vector, at every source index j
        for (int j = 0; j < n; ++j) {
          auto v = build<T>(n, spare);
          Before<T> keep(v, p);
          auto it = v.insert(v.begin() + p, v[static_cast<std::size_t>(j)]);
          model_init(n);
          model_insert(p, {10 + j});
          CHECK(it == v.begin() + p);
          expect(v, model, model_n);
          keep.check(v);
          // emplace(p, element)
          auto w = build<T>(n, spare);
          auto it2 = w.emplace(w.begin() + p, w[static_cast<std::size_t>(j)]);
          CHECK(it2 == w.begin() + p);
          expect(w, model, model_n);
          // insert(p, k, element)
          for (int k = 0; k <= 3; ++k) {
            auto u = build<T>(n, spare);
            auto it3 = u.insert(u.begin() + p, static_cast<std::size_t>(k), u[static_cast<std::size_t>(j)]);
            CHECK(it3 == u.begin() + p);
            model_init(n);
            for (int r = 0; r < k; ++r) model_insert(p, {10 + j});
            expect(u, model, model_n);
          }
        }
        // insert(p, first, last) from another array of every length; insert(p, il)
        for (int len = 0; len <= 5; ++len) {
          T src[5] = {make<T>(100), make<T>(101), make<T>(102), make<T>(103), make<T>(104)};
          auto v = build<T>(n, spare);
          Before<T> keep(v, p);
          auto it = v.insert(v.begin() + p, src, src + len);
          CHECK(it == v.begin() + p);
          model_init(n);
          for (int r = len - 1; r >= 0; --r) model_insert(p, {100 + r});
          expect(v, model, model_n);
          keep.check(v);
        }
        {
          auto v = build<T>(n, spare);
          auto it = v.insert(v.begin() + p, {make<T>(200), make<T>(201), make<T>(202)});
          CHECK(it == v.begin() + p);
          model_init(n);
          model_insert(p, {200, 201, 202});
          expect(v, model, model_n);
        }
        {
          auto v = build<T>(n, spare);
          T x = make<T>(300);
          auto it = v.insert(v.begin() + p, static_cast<T&&>(x));
          CHECK(it == v.begin() + p);
          model_init(n);
          model_insert(p, {300});
          expect(v, model, model_n);
        }
      }
}

template <class T>
void erases() {
  for (int n = 0; n <= MAXN; ++n)
    for (int f = 0; f <= n; ++f)
      for (int l = f; l <= n; ++l) {
        auto v = build<T>(n, f % 2 == 0);
        const T* data = v.data();
        long a0 = assigns, d0 = dtors;
        auto it = v.erase(v.begin() + f, v.begin() + l);
        if constexpr (std::is_same_v<T, AssignCounted>) if (f < l) CHECK(assigns - a0 == n - l);
        if constexpr (std::is_same_v<T, DtorCounted>) CHECK(dtors - d0 == l - f);
        CHECK(it == v.begin() + f);
        CHECK(v.data() == data);  // erase never reallocates (/4: only invalidates at/after)
        model_init(n);
        model_erase(f, l);
        expect(v, model, model_n);
        if (f < n && l == f + 1) {
          auto w = build<T>(n, true);
          long a1 = assigns, d1 = dtors;
          auto it2 = w.erase(w.cbegin() + f);
          if constexpr (std::is_same_v<T, AssignCounted>) CHECK(assigns - a1 == n - f - 1);
          if constexpr (std::is_same_v<T, DtorCounted>) CHECK(dtors - d1 == 1);
          CHECK(it2 == w.begin() + f);
          expect(w, model, model_n);
        }
      }
  // erase repeatedly from a long vector, every position in turn
  for (int start = 0; start < 3; ++start) {
    auto v = build<T>(40, true);
    model_init(40);
    int p = start;
    while (model_n > 0) {
      p = (p * 7 + 3) % model_n;
      v.erase(v.begin() + p);
      model_erase(p, p + 1);
      expect(v, model, model_n);
    }
  }
}

// Trivially copyable, not assignable: growth at the end only.
struct ConstMember {
  const int a;
  int b;
};
struct RefMember {
  int& r;
  int b;
};
static_assert(std::is_trivially_copyable_v<ConstMember> && !std::is_copy_assignable_v<ConstMember>);
static_assert(std::is_trivially_copyable_v<RefMember> && !std::is_copy_assignable_v<RefMember>);

static int cells[200];

static void non_assignable() {
  std::vector<ConstMember> c;
  std::vector<RefMember> r;
  for (int i = 0; i < 200; ++i) {
    cells[i] = 1000 + i;
    if (i % 2) c.push_back(ConstMember{i, -i});
    else c.emplace_back(i, -i);
    r.push_back(RefMember{cells[i], i});
    if (i == 50) {
      c.shrink_to_fit();
      r.shrink_to_fit();
    }
    if (i == 120) {
      c.reserve(1000);
      r.reserve(1000);
    }
  }
  for (int i = 0; i < 200; ++i) {
    CHECK(c[i].a == i && c[i].b == -i);
    CHECK(&r[i].r == &cells[i] && r[i].b == i);
  }
  std::vector<ConstMember> c2 = c;
  std::vector<RefMember> r2(std::move(r));
  CHECK(c2.size() == 200 && r2.size() == 200);
  for (int i = 0; i < 200; ++i) {
    CHECK(c2[i].a == i && c2[i].b == -i);
    CHECK(&r2[i].r == &cells[i]);
  }
  c2.pop_back();
  c2.resize(250, ConstMember{7, 8});
  CHECK(c2[198].a == 198 && c2[199].a == 7 && c2[249].b == 8);
  c2.resize(10, ConstMember{0, 0});  // (not default-insertable)
  CHECK(c2.size() == 10 && c2[9].a == 9);
  c2.swap(c);
  CHECK(c.size() == 10 && c2.size() == 200);
}

template <class T>
void all() {
  inserts<T>();
  erases<T>();
}

int main() {
  all<int>();
  all<unsigned char>();
  all<double>();
  all<Pad3>();
  all<Big>();
  all<AssignCounted>();
  all<DtorCounted>();
  non_assignable();
}
