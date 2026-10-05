// Exception-injection sweep over shared_ptr's owning constructors, reset, assignment from
// unique_ptr, make_shared/allocate_shared (objects and arrays): operator new, exh::alloc's
// allocate, and the element type's constructors throw at their k-th call, for every k.
//   [util.smartptr.shared.const]/10: shared_ptr(p): "If an exception is thrown, delete p is
//     called." /14 (shared_ptr(p, d), (p, d, a), (nullptr, d), ...): "If an exception is thrown,
//     d(p) is called." /29: shared_ptr(unique_ptr&&): "If an exception is thrown, the
//     constructor has no effect" (r still owns its pointer).
//   [util.smartptr.shared.mod]/3-5: reset(p[, d[, a]]) is shared_ptr(p, ...).swap(*this): on an
//     exception *this is unchanged and p is deleted / d(p) called.
//   [util.smartptr.shared.assign]/6: operator=(unique_ptr&&) is shared_ptr(std::move(r)).swap(*this).
//   [util.smartptr.shared.create]/3: "If an exception is thrown, the functions have no
//     effect"; /7.9-7.10: "Array elements are initialized in ascending order of their
//     addresses"; "when the initialization of an array element throws an exception, the
//     initialized elements are destroyed in the reverse order of their original construction."
// After every run all objects are destroyed exactly once and every block (operator new and
// exh::alloc) freed.
// REQUIRES: exceptions
#include <memory>
#include "exc_new.hpp"

using namespace exh;

// Records construction and destruction order (ids are addresses' ordinal in construction).
struct Ord {
  static inline const void* built[64];
  static inline int nbuilt = 0;
  static inline bool order_ok = true;
  static inline int live = 0;
  Ord() {
    point(default_ctor);
    if (nbuilt > 0 && !(built[nbuilt - 1] < static_cast<const void*>(this))) order_ok = false; // ascending addresses
    built[nbuilt++] = this;
    ++live;
  }
  Ord(const Ord&) : Ord() {}
  ~Ord() {
    // destroyed in reverse order of construction
    int i = nbuilt - 1;
    while (i >= 0 && built[i] != this) --i;
    if (i != nbuilt - 1) order_ok = false;
    if (i >= 0) {
      for (int j = i; j + 1 < nbuilt; ++j) built[j] = built[j + 1];
      --nbuilt;
    }
    --live;
  }
};

static const T x_g(3);                 // created before any sweep
static const T init_g[2] = {T(1), T(2)};

struct Del {
  int* calls;
  void operator()(T* p) const {
    ++*calls;
    delete p;
  }
};

template <class F>
void sw(const char* name, std::initializer_list<Kind> ks, F f) {
  for (Kind k : ks) {
    if (k == gnew)
      sweep_new(name, f);
    else
      sweep(name, k, new_balanced(f));
  }
}

int main() {
  sw("shared_ptr(new T)", {gnew}, [] {
    T* p = new T(1);
    long live0 = st.live;
    bool threw = attempt([&] { std::shared_ptr<T> s(p); });
    if (threw) EXH_EXPECT(st.live == live0 - 1, "shared_ptr(p) threw without deleting p");
    return threw;
  });
  sw("shared_ptr(p, d)", {gnew}, [] {
    int calls = 0;
    T* p = new T(1);
    bool threw = attempt([&] { std::shared_ptr<T> s(p, Del{&calls}); });
    EXH_EXPECT(calls == 1, "d(p) not called exactly once");
    return threw;
  });
  sw("shared_ptr(p, d, a)", {allocation}, [] {
    int calls = 0;
    T* p = new T(1);
    bool threw = attempt([&] { std::shared_ptr<T> s(p, Del{&calls}, alloc<int>()); });
    EXH_EXPECT(calls == 1, "d(p) not called exactly once");
    return threw;
  });
  sw("shared_ptr(nullptr, d, a)", {allocation}, [] {
    int calls = 0;
    bool threw = attempt([&] {
      std::shared_ptr<T> s(nullptr, [&](T* q) { ++calls; delete q; }, alloc<int>());
    });
    EXH_EXPECT(calls == 1, "d(nullptr) not called exactly once");
    return threw;
  });
  sw("shared_ptr(unique_ptr&&)", {gnew}, [] {
    auto u = std::make_unique<T>(1);
    T* raw = u.get();
    bool threw = attempt([&] { std::shared_ptr<T> s(std::move(u)); });
    if (threw) EXH_EXPECT(u.get() == raw, "shared_ptr(unique_ptr&&) had an effect although it threw");
    return threw;
  });
  sw("shared_ptr = unique_ptr&&", {gnew}, [] {
    auto u = std::make_unique<T>(1);
    T* raw = u.get();
    auto s = std::make_shared<T>(2);
    T* old = s.get();
    bool threw = attempt([&] { s = std::move(u); });
    if (threw) {
      EXH_EXPECT(u.get() == raw, "unique_ptr lost ownership although the assignment threw");
      EXH_EXPECT(s.get() == old && s->v == 2 && s.use_count() == 1, "shared_ptr changed although the assignment threw");
    }
    return threw;
  });
  sw("reset(p)", {gnew}, [] {
    auto s = std::make_shared<T>(2);
    T* old = s.get();
    long live0 = st.live;
    T* p = new T(1);
    bool threw = attempt([&] { s.reset(p); });
    if (threw) {
      EXH_EXPECT(st.live == live0, "reset(p) threw without deleting p");
      EXH_EXPECT(s.get() == old && s->v == 2, "reset(p) changed *this although it threw");
    }
    return threw;
  });
  sw("reset(p, d, a)", {allocation}, [] {
    int calls = 0; // outlives s, whose deleter may refer to it
    auto s = std::make_shared<T>(2);
    T* old = s.get();
    T* p = new T(1);
    bool threw = attempt([&] { s.reset(p, Del{&calls}, alloc<int>()); });
    if (threw) {
      EXH_EXPECT(calls == 1, "reset(p, d, a) threw without calling d(p)");
      EXH_EXPECT(s.get() == old, "reset(p, d, a) changed *this although it threw");
    }
    return threw;
  });
  sw("make_shared<T>(int)", {gnew, value_ctor}, [] { return attempt([] { auto s = std::make_shared<T>(5); }); });
  sw("allocate_shared<T>(a, int)", {allocation, value_ctor},
     [] { return attempt([] { auto s = std::allocate_shared<T>(alloc<T>(), 5); }); });
  sw("make_shared<T[]>(8)", {gnew, default_ctor}, [] { return attempt([] { auto s = std::make_shared<T[]>(8); }); });
  sw("make_shared<T[]>(8, x)", {gnew, copy_ctor}, [] {
    return attempt([] { auto s = std::make_shared<T[]>(8, x_g); });
  });
  sw("make_shared<T[5]>()", {gnew, default_ctor}, [] { return attempt([] { auto s = std::make_shared<T[5]>(); }); });
  sw("make_shared<T[][3]>(4)", {gnew, default_ctor}, [] { return attempt([] { auto s = std::make_shared<T[][3]>(4); }); });
  sw("allocate_shared<T[]>(a, 8)", {allocation, default_ctor},
     [] { return attempt([] { auto s = std::allocate_shared<T[]>(alloc<T>(), 8); }); });
  sw("allocate_shared<T[][2]>(a, 3, {x, y})", {allocation, copy_ctor}, [] {
    return attempt([] { auto s = std::allocate_shared<T[][2]>(alloc<T>(), 3, init_g); });
  });
  sw("make_shared_for_overwrite<T[]>(6)", {gnew, default_ctor},
     [] { return attempt([] { auto s = std::make_shared_for_overwrite<T[]>(6); }); });

  // Construction order and reverse-order destruction of array elements.
  for (int which = 0; which < 3; ++which)
    sweep(which == 0 ? "make_shared<Ord[]>(10) order" : which == 1 ? "make_shared<Ord[2][3]>() order"
                                                                    : "allocate_shared<Ord[]>(a, 10) order",
          default_ctor, [which] {
            Ord::nbuilt = 0;
            Ord::order_ok = true;
            bool threw = attempt([which] {
              if (which == 0) {
                auto s = std::make_shared<Ord[]>(10);
              } else if (which == 1) {
                auto s = std::make_shared<Ord[2][3]>();
              } else {
                auto s = std::allocate_shared<Ord[]>(alloc<Ord>(), 10);
              }
            });
            EXH_EXPECT(Ord::order_ok, "[util.smartptr.shared.create]/7.9-7.10: construction or destruction order");
            EXH_EXPECT(Ord::live == 0 && Ord::nbuilt == 0, "array elements not all destroyed");
            Ord::live = 0;
            return threw;
          });
  return finish();
}
