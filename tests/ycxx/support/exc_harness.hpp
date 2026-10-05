// Exception-injection and resource-accounting harness for libycxx's own suite.
// Written from [res.on.exception.handling], [allocator.requirements], [iterator.requirements]
// and [container.reqmts]; deliberately independent of every other test suite.
//
// Every user-supplied operation an algorithm or container can call is an injection point of
// some Kind. A sweep runs a scenario for k = 1, 2, ...: during the scenario's guarded call the
// k-th operation of the chosen Kind throws, until a run completes without throwing. After
// every run the harness checks exact resource accounting:
//   - every Tracked object constructed has been destroyed exactly once (live count back to its
//     starting value; a destructor or member called on a dead object is reported as misuse);
//   - every block obtained from exh::alloc has been returned exactly once, with the same n and
//     the same value_type size ([allocator.requirements.general]: deallocate(p, n): "p has
//     been returned by a prior call to allocate ... n equals the value passed").
// The scenario itself checks the guarantee the draft states for the operation (strong /
// no effects / basic) through EXH_EXPECT, typically with exh::snap.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <compare>
#include <initializer_list>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace exh {

enum Kind : int {
  copy_ctor,    // Tracked(const Tracked&)
  move_ctor,    // Tracked(Tracked&&) (only for the potentially-throwing variant)
  copy_assign,  // operator=(const Tracked&)
  move_assign,  // operator=(Tracked&&) (only for the potentially-throwing variant)
  default_ctor, // Tracked()
  value_ctor,   // Tracked(int)
  iter_inc,     // ++, --, +=, -= of exh::iter
  iter_deref,   // *, [] of exh::iter
  iter_cmp,     // ==, <=>, and difference of exh::iter
  allocation,   // exh::alloc<T>::allocate
  hash,         // exh::hasher
  compare,      // exh::less, exh::equal
  pred,         // exh::pred_odd, exh::pred_less
  gnew,         // replaced global operator new (exc_new.hpp only)
  virt,         // a user-overridden virtual function (streambuf, locale facets)
  n_kinds
};
inline const char* const kind_name[n_kinds] = {"copy_ctor", "move_ctor", "copy_assign", "move_assign",
                                               "default_ctor", "value_ctor", "iter_inc", "iter_deref",
                                               "iter_cmp", "alloc", "hash", "compare", "pred", "gnew",
                                               "virtual"};

// The exception thrown at an injection point. Allocation points throw alloc_failure, which is
// a bad_alloc ([allocator.requirements.general]; [new.delete.single]/3: a replaceable operator
// new reports failure only by bad_alloc).
struct Injected {
  Kind kind;
};
struct alloc_failure : std::bad_alloc {
  Kind kind;
  explicit alloc_failure(Kind k) noexcept : kind(k) {}
  const char* what() const noexcept override { return "exh::alloc_failure"; }
};

struct State {
  long left[n_kinds] = {}; // 0: disarmed; otherwise the operation that brings it to 0 throws
  long calls[n_kinds] = {};
  bool fired = false;
  long live = 0; // Tracked objects alive
  long failures = 0;
  long printed = 0;
  const char* scenario = "(none)";
  long runs = 0, throwing_runs = 0, sweeps = 0;
  int kind = -1;
  long k = 0;
};
inline State st;

inline void report(const char* what, int line) {
  ++st.failures;
  if (st.printed++ < 60)
    dprintf(2, "FAIL line %d: [%s] kind=%s k=%ld: %s\n", line, st.scenario,
            st.kind >= 0 ? kind_name[st.kind] : "-", st.k, what);
}
#define EXH_EXPECT(cond, what)                                                                       \
  do {                                                                                               \
    if (!(cond)) ::exh::report(what " (" #cond ")", __LINE__);                                        \
  } while (0)

inline bool should_throw(Kind k) {
  ++st.calls[k];
  if (st.left[k] > 0 && --st.left[k] == 0) {
    st.fired = true;
    return true;
  }
  return false;
}
inline void point(Kind k) {
  if (should_throw(k)) throw Injected{k};
}
inline void alloc_point(Kind k) {
  if (should_throw(k)) throw alloc_failure(k);
}
inline void disarm() {
  for (auto& l : st.left) l = 0;
}

// ---------------------------------------------------------------------------------------------
// Tracked element.
constexpr unsigned alive_magic = 0x5AFE1234u, dead_magic = 0xDEADBEEFu;
constexpr int moved_from = -7777;

template <bool NothrowMove>
struct basic_tracked {
  int v;
  unsigned magic;
  basic_tracked() : v(0) {
    point(default_ctor);
    born();
  }
  basic_tracked(int x) : v(x) {
    point(value_ctor);
    born();
  }
  basic_tracked(const basic_tracked& o) : v(o.v) {
    o.use("copy from");
    point(copy_ctor);
    born();
  }
  basic_tracked(basic_tracked&& o) noexcept(NothrowMove) : v(o.v) {
    o.use("move from");
    if constexpr (!NothrowMove) point(move_ctor);
    o.v = moved_from;
    born();
  }
  basic_tracked& operator=(const basic_tracked& o) {
    use("copy-assign to");
    o.use("copy-assign from");
    point(copy_assign);
    v = o.v;
    return *this;
  }
  basic_tracked& operator=(basic_tracked&& o) noexcept(NothrowMove) {
    use("move-assign to");
    o.use("move-assign from");
    if constexpr (!NothrowMove) point(move_assign);
    int t = o.v;
    o.v = moved_from;
    v = t;
    return *this;
  }
  ~basic_tracked() {
    use("destroy");
    magic = dead_magic;
    --st.live;
  }
  void born() {
    magic = alive_magic;
    ++st.live;
  }
  void use(const char* what) const {
    if (magic != alive_magic) {
      char buf[96];
      __builtin_snprintf(buf, sizeof buf, "%s an object that is not alive (magic %#x)", what, magic);
      report_dyn(buf);
    }
  }
  static void report_dyn(const char* s) {
    ++st.failures;
    if (st.printed++ < 60)
      dprintf(2, "FAIL: [%s] kind=%s k=%ld: %s\n", st.scenario, st.kind >= 0 ? kind_name[st.kind] : "-", st.k, s);
  }
  friend bool operator==(const basic_tracked& a, const basic_tracked& b) { return a.v == b.v; }
  friend std::strong_ordering operator<=>(const basic_tracked& a, const basic_tracked& b) { return a.v <=> b.v; }
};
using T = basic_tracked<false>;  // every special member can throw
using NT = basic_tracked<true>;  // noexcept move constructor and move assignment

// ---------------------------------------------------------------------------------------------
// Counting, throwing allocator with a block registry.
struct Block {
  void* p;
  std::size_t n;
  std::size_t elem;
  int id; // id of the allocating exh::alloc (-1: not tracked)
};
inline Block blocks[16384];
inline int nblocks = 0;
inline long alloc_calls = 0;

inline void* registry_alloc(std::size_t n, std::size_t elem, std::size_t align, int id = -1) {
  std::size_t bytes = n * elem;
  if (bytes == 0) bytes = 1;
  if (align < alignof(std::max_align_t)) align = alignof(std::max_align_t);
  bytes = (bytes + align - 1) / align * align;
  void* p = std::aligned_alloc(align, bytes);
  if (!p) throw std::bad_alloc();
  if (nblocks == int(sizeof blocks / sizeof blocks[0])) {
    report("allocator registry full", __LINE__);
    return p;
  }
  blocks[nblocks++] = Block{p, n, elem, id};
  ++alloc_calls;
  return p;
}
inline void registry_dealloc(void* p, std::size_t n, std::size_t elem, int id = -1) {
  for (int i = nblocks - 1; i >= 0; --i)
    if (blocks[i].p == p) {
      if (id != -1 && blocks[i].id != -1 && blocks[i].id != id) {
        char buf[160];
        __builtin_snprintf(buf, sizeof buf,
                           "deallocate by an allocator (id %d) that does not compare equal to the allocating one (id %d) (for a memory_resource: another alignment)",
                           id, blocks[i].id);
        basic_tracked<false>::report_dyn(buf);
      }
      if (blocks[i].n != n || blocks[i].elem != elem) {
        char buf[160];
        __builtin_snprintf(buf, sizeof buf,
                           "deallocate(p, %zu) of value size %zu for a block allocated as %zu x %zu", n, elem,
                           blocks[i].n, blocks[i].elem);
        basic_tracked<false>::report_dyn(buf);
      }
      blocks[i] = blocks[--nblocks];
      std::free(p);
      return;
    }
  basic_tracked<false>::report_dyn("deallocate of a pointer not obtained from allocate (or deallocated twice)");
}

// propagate_on_container_* are all Prop; allocators compare equal iff their ids are equal.
template <class V, bool Prop = true>
struct alloc {
  using value_type = V;
  using propagate_on_container_copy_assignment = std::bool_constant<Prop>;
  using propagate_on_container_move_assignment = std::bool_constant<Prop>;
  using propagate_on_container_swap = std::bool_constant<Prop>;
  using is_always_equal = std::false_type;
  template <class U>
  struct rebind {
    using other = alloc<U, Prop>;
  };
  int id = 1;
  alloc() noexcept = default;
  explicit alloc(int i) noexcept : id(i) {}
  template <class U>
  alloc(const alloc<U, Prop>& o) noexcept : id(o.id) {}
  V* allocate(std::size_t n) {
    alloc_point(allocation);
    return static_cast<V*>(registry_alloc(n, sizeof(V), alignof(V), id));
  }
  // [allocator.requirements.general]: a.deallocate(p, n): "p has been returned by a prior call
  // to allocate on an allocator that compares equal to a"
  void deallocate(V* p, std::size_t n) noexcept { registry_dealloc(p, n, sizeof(V), id); }
  template <class U>
  friend bool operator==(const alloc& a, const alloc<U, Prop>& b) noexcept {
    return a.id == b.id;
  }
};

// ---------------------------------------------------------------------------------------------
// Throwing iterator over an array of V (Cat: input/forward/bidirectional/random_access tag).
template <class Cat, class V = T>
struct iter {
  using iterator_category = Cat;
  using iterator_concept = Cat;
  using value_type = V;
  using difference_type = std::ptrdiff_t;
  using pointer = const V*;
  using reference = const V&;
  static constexpr bool bidi = std::is_base_of_v<std::bidirectional_iterator_tag, Cat>;
  static constexpr bool ra = std::is_base_of_v<std::random_access_iterator_tag, Cat>;
  const V* p = nullptr;
  iter() = default;
  explicit iter(const V* q) : p(q) {}
  reference operator*() const {
    point(iter_deref);
    return *p;
  }
  pointer operator->() const {
    point(iter_deref);
    return p;
  }
  iter& operator++() {
    point(iter_inc);
    ++p;
    return *this;
  }
  iter operator++(int) {
    iter t = *this;
    ++*this;
    return t;
  }
  iter& operator--()
    requires bidi
  {
    point(iter_inc);
    --p;
    return *this;
  }
  iter operator--(int)
    requires bidi
  {
    iter t = *this;
    --*this;
    return t;
  }
  iter& operator+=(difference_type n)
    requires ra
  {
    point(iter_inc);
    p += n;
    return *this;
  }
  iter& operator-=(difference_type n)
    requires ra
  {
    point(iter_inc);
    p -= n;
    return *this;
  }
  friend iter operator+(iter a, difference_type n)
    requires ra
  {
    return a += n;
  }
  friend iter operator+(difference_type n, iter a)
    requires ra
  {
    return a += n;
  }
  friend iter operator-(iter a, difference_type n)
    requires ra
  {
    return a -= n;
  }
  friend difference_type operator-(const iter& a, const iter& b)
    requires ra
  {
    point(iter_cmp);
    return a.p - b.p;
  }
  reference operator[](difference_type n) const
    requires ra
  {
    point(iter_deref);
    return p[n];
  }
  friend bool operator==(const iter& a, const iter& b) {
    point(iter_cmp);
    return a.p == b.p;
  }
  friend std::strong_ordering operator<=>(const iter& a, const iter& b)
    requires ra
  {
    point(iter_cmp);
    return a.p <=> b.p;
  }
};
using in_tag = std::input_iterator_tag;
using fwd_tag = std::forward_iterator_tag;
using bidi_tag = std::bidirectional_iterator_tag;
using ra_tag = std::random_access_iterator_tag;

// A range over [b, e) as exh::iter (subrange-like, without depending on <ranges>).
template <class Cat, class V = T>
struct range {
  const V* b;
  const V* e;
  iter<Cat, V> begin() const { return iter<Cat, V>(b); }
  iter<Cat, V> end() const { return iter<Cat, V>(e); }
};

// ---------------------------------------------------------------------------------------------
// Throwing function objects.
struct hasher {
  template <class X>
  std::size_t operator()(const X& x) const {
    point(hash);
    return std::size_t(unsigned(x.v)) * 2654435761u;
  }
};
struct less {
  template <class X, class Y>
  bool operator()(const X& a, const Y& b) const {
    point(compare);
    return a.v < b.v;
  }
};
struct greater {
  template <class X, class Y>
  bool operator()(const X& a, const Y& b) const {
    point(compare);
    return a.v > b.v;
  }
};
struct equal {
  template <class X, class Y>
  bool operator()(const X& a, const Y& b) const {
    point(compare);
    return a.v == b.v;
  }
};
struct pred_odd {
  template <class X>
  bool operator()(const X& a) const {
    point(pred);
    return a.v % 2 != 0;
  }
};

// ---------------------------------------------------------------------------------------------
// Snapshots of observable values, independent of the library under test.
inline int valof(int x) { return x; }
inline int valof(char x) { return x; }
inline int valof(bool x) { return x; }
template <bool B>
int valof(const basic_tracked<B>& t) {
  return t.v;
}
template <class A, class B>
int valof(const std::pair<A, B>& p) {
  return valof(p.first) * 1000 + valof(p.second);
}
// Proxy references (vector<bool>, flat_map's pair of references).
template <class R>
  requires requires(const R& r) { r.first; r.second; }
int valof(const R& r) {
  return valof(r.first) * 1000 + valof(r.second);
}
template <class R>
  requires(std::is_class_v<R> && std::is_convertible_v<const R&, bool> && !requires(const R& r) { r.v; } &&
           !requires(const R& r) { r.first; })
int valof(const R& r) {
  return bool(r);
}

struct snap {
  int n = 0;
  int v[1024];
  template <class R>
  static snap of(const R& r) {
    snap s;
    for (auto&& x : r) {
      if (s.n < 1024) s.v[s.n] = valof(x);
      ++s.n;
    }
    return s;
  }
  snap sorted() const {
    snap s = *this;
    for (int i = 1; i < s.n && i < 1024; ++i)
      for (int j = i; j > 0 && s.v[j - 1] > s.v[j]; --j) {
        int t = s.v[j];
        s.v[j] = s.v[j - 1];
        s.v[j - 1] = t;
      }
    return s;
  }
  bool is_sorted(bool strict) const {
    for (int i = 1; i < n && i < 1024; ++i)
      if (strict ? !(v[i - 1] < v[i]) : v[i] < v[i - 1]) return false;
    return true;
  }
  friend bool operator==(const snap& a, const snap& b) {
    if (a.n != b.n) return false;
    for (int i = 0; i < a.n && i < 1024; ++i)
      if (a.v[i] != b.v[i]) return false;
    return true;
  }
};

// ---------------------------------------------------------------------------------------------
// Driver.

// Runs f() with the current kind armed at the current k. Returns true iff an injected
// exception escaped f. Any other exception propagates.
template <class F>
bool attempt(F&& f) {
  disarm();
  st.fired = false;
  if (st.kind >= 0) st.left[st.kind] = st.k;
  bool threw = false;
  try {
    f();
  } catch (const Injected& e) {
    threw = true;
    if (e.kind != st.kind) report("an injected exception of another kind escaped", __LINE__);
  } catch (const alloc_failure& e) {
    threw = true;
    if (e.kind != st.kind) report("an injected allocation failure of another kind escaped", __LINE__);
  }
  disarm();
  return threw;
}

struct options {
  bool may_swallow = false; // an injected failure may be handled internally (e.g. a temporary
                            // buffer allocation, [alg.sort]'s "if enough extra memory")
  long max_k = 4000;
  long step = 1; // inject at k = 1, 1 + step, ...: for operations that allocate many times
};

// Calls scenario() for k = 1, 2, ... (scenario returns attempt(...)'s result), checking resource
// accounting after each run, until a run completes without an injected exception.
template <class F>
void sweep(const char* name, Kind kind, F&& scenario, options o = {}) {
  st.scenario = name;
  st.kind = kind;
  ++st.sweeps;
  for (long k = 1; k <= o.max_k; k += o.step) {
    st.k = k;
    const long live0 = st.live;
    const int blocks0 = nblocks;
    bool threw = scenario();
    disarm();
    ++st.runs;
    st.throwing_runs += threw;
    if (st.live != live0) {
      char buf[128];
      __builtin_snprintf(buf, sizeof buf, "%ld element object(s) %s after the run", st.live - live0 > 0 ? st.live - live0 : live0 - st.live,
                         st.live > live0 ? "leaked" : "destroyed too often");
      basic_tracked<false>::report_dyn(buf);
      st.live = live0;
    }
    if (nblocks != blocks0) {
      char buf[128];
      __builtin_snprintf(buf, sizeof buf, "%d allocator block(s) not deallocated after the run", nblocks - blocks0);
      basic_tracked<false>::report_dyn(buf);
      nblocks = blocks0; // (leaked memory is not reused)
    }
    if (!threw) {
      if (st.fired && !o.may_swallow) report("the injected exception did not propagate", __LINE__);
      st.kind = -1;
      st.scenario = "(none)";
      return;
    }
  }
  report("the operation never completed", __LINE__);
  st.kind = -1;
}

template <class F>
void sweep_kinds(const char* name, std::initializer_list<Kind> kinds, F&& scenario, options o = {}) {
  for (Kind k : kinds) sweep(name, k, scenario, o);
}

inline int finish() {
  dprintf(1, "%ld sweeps, %ld runs, %ld with an injected exception\n", st.sweeps, st.runs, st.throwing_runs);
  if (st.failures) {
    dprintf(2, "%ld failure(s)\n", st.failures);
    return 1;
  }
  return 0;
}

} // namespace exh
