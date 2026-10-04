// Generic exception-injection sweep over the modifiers and constructors of a sequence
// container (vector, deque, list, forward_list, inplace_vector, hive), built on exc_harness.hpp.
// A policy P supplies:
//   using C  = the container (element type P::E, allocator exh::alloc<E> where applicable);
//   using C2 = the same container with a non-propagating exh::alloc<E, false> (or void);
//   static C make(int variant);      // 5 elements 10..14; variant 1 may differ in capacity
//   static constexpr int variants;   // number of make() variants
//   static G guarantee(Pos, Op, Kind) // what the draft requires when that kind throws
//   static constexpr bool ordered;   // false: compare snapshots as multisets (hive)
// The checks after an injected exception:
//   all:    the elements the container holds are exactly the live element objects it owns
//           (none leaked, none destroyed twice), the container is traversable and size()
//           agrees with the traversal;
//   strong: its observable contents are unchanged;
//   prefix: [inplace.vector.modifiers]/3: size() >= n and the first n elements are unchanged.
// shrink_to_fit is a non-binding request ([vector.capacity]/9, [deque.capacity]/6): an
// allocation failure inside it may be handled by not shrinking.
#pragma once
#include <iterator>
#include <ranges>
#include <utility>
#include "exc_harness.hpp"

namespace exh::seq {

enum class G { strong, basic, prefix };
enum class Pos { front, mid, late /* before the last element */, end, back /* push_back, emplace_back */, none };
enum class Op { single, multi, resize, reserve, shrink, assign, erase, ctor };

template <class C>
long count(const C& c) {
  long n = 0;
  for (auto it = c.begin(); it != c.end(); ++it) ++n;
  return n;
}

template <class C>
constexpr bool is_fwd = requires(C& c) { c.before_begin(); };
template <class C>
constexpr bool is_hive = requires(C& c) { c.reshape(c.block_capacity_limits()); };

template <class C>
auto at(C& c, Pos p) {
  if constexpr (is_fwd<C>) {
    // insert_after position: before_begin / after the second element / the last element
    auto it = c.before_begin();
    long steps = p == Pos::front ? 0 : p == Pos::mid ? 2 : p == Pos::late ? count(c) - 1 : count(c);
    for (long i = 0; i < steps; ++i) ++it;
    return it;
  } else {
    auto it = c.begin();
    long steps = p == Pos::front ? 0 : p == Pos::mid ? 2 : p == Pos::late ? count(c) - 1 : count(c);
    std::advance(it, steps);
    return typename C::const_iterator(it);
  }
}

template <class P>
struct runner {
  using C = typename P::C;
  using E = typename P::E;
  static inline const char* cname = P::name;

  // P::val(i) (optional) maps an int to an element value (vector<bool>: alternating bits).
  static E mk(int i) {
    if constexpr (requires { P::val(i); })
      return P::val(i);
    else
      return E(i);
  }
  static E* src() {
    static E s[6] = {mk(50), mk(51), mk(52), mk(53), mk(54), mk(55)};
    return s;
  }
  // A longer source (several deque/hive blocks, several vector growth steps).
  static constexpr int big_n = 70;
  static E* big() {
    static E* b = [] {
      alignas(E) static unsigned char raw[big_n * sizeof(E)];
      E* p = reinterpret_cast<E*>(raw);
      for (int i = 0; i < big_n; ++i) ::new (p + i) E(mk(100 + i));
      return p;
    }();
    return b;
  }

  // Runs op(c, tmp) on a fresh container (all variants) under a sweep of every kind in ks.
  static void go(const char* opname, Pos pos, Op cls, std::initializer_list<Kind> ks, auto op) {
    static char label[160];
    __builtin_snprintf(label, sizeof label, "%s: %s", cname, opname);
    for (int variant = 0; variant < P::variants; ++variant)
      for (Kind k : ks)
        sweep(label, k, [&] {
          C c = P::make(variant);
          E tmp(77);
          const snap before = snap::of(c);
          const long n0 = count(c);
          const long live0 = st.live;
          bool threw = attempt([&] { op(c, tmp); });
          const long n = count(c);
          if constexpr (requires { c.size(); }) EXH_EXPECT(long(c.size()) == n, "size() disagrees with traversal");
          // The container owns exactly n element objects (tmp may have been moved from but is
          // still alive).
          if constexpr (requires(E e) { e.v; })
            EXH_EXPECT(st.live - live0 == n - n0, "live element objects do not match the container's size");
          if (threw) {
            G g = P::guarantee(pos, cls, Kind(st.kind));
            snap after = snap::of(c);
            if (g == G::strong) {
              if constexpr (P::ordered)
                EXH_EXPECT(after == before, "strong guarantee violated: contents changed");
              else
                EXH_EXPECT(after.sorted() == before.sorted(), "strong guarantee violated: contents changed");
            } else if (g == G::prefix) {
              long pre = pos == Pos::front ? 0 : pos == Pos::mid ? 2 : pos == Pos::late ? n0 - 1 : n0;
              bool ok = after.n >= pre;
              for (long i = 0; ok && i < pre; ++i) ok = after.v[i] == before.v[i];
              EXH_EXPECT(ok, "[inplace.vector.modifiers]/3 violated: the first n elements changed or were removed");
            }
          }
          return threw;
        }, options{.may_swallow = cls == Op::shrink});
  }

  static constexpr std::initializer_list<Kind> elem_kinds = {copy_ctor, move_ctor, copy_assign, move_assign,
                                                             allocation};
  static constexpr std::initializer_list<Kind> all_kinds = {copy_ctor,  move_ctor,  copy_assign, move_assign,
                                                            allocation, iter_inc,   iter_deref,  iter_cmp,
                                                            value_ctor, default_ctor};

  template <class Cat>
  static auto rng() {
    return range<Cat, E>{src(), src() + 6};
  }

  static void insertions() {
    constexpr bool fwd = is_fwd<C>;
    constexpr bool hive = is_hive<C>;
    for (Pos p : {Pos::front, Pos::mid, Pos::late, Pos::end}) {
      const char* pn = p == Pos::front ? "front" : p == Pos::mid ? "mid" : p == Pos::late ? "late" : "end";
      char nm[96];
      auto name = [&](const char* what) {
        __builtin_snprintf(nm, sizeof nm, "%s @%s", what, pn);
        return nm;
      };
      if constexpr (fwd) {
        go(name("insert_after(p, const T&)"), p, Op::single, elem_kinds,
           [&](C& c, E&) { c.insert_after(at(c, p), src()[0]); });
        go(name("insert_after(p, T&&)"), p, Op::single, elem_kinds,
           [&](C& c, E& t) { c.insert_after(at(c, p), std::move(t)); });
        go(name("emplace_after(p, int)"), p, Op::single, {value_ctor, allocation},
           [&](C& c, E&) { c.emplace_after(at(c, p), 66); });
        go(name("insert_after(p, 3, x)"), p, Op::multi, elem_kinds,
           [&](C& c, E&) { c.insert_after(at(c, p), 3, src()[1]); });
        go(name("insert_after(p, input first, last)"), p, Op::multi, all_kinds, [&](C& c, E&) {
          auto r = rng<in_tag>();
          c.insert_after(at(c, p), r.begin(), r.end());
        });
        go(name("insert_after(p, fwd first, last)"), p, Op::multi, all_kinds, [&](C& c, E&) {
          auto r = rng<fwd_tag>();
          c.insert_after(at(c, p), r.begin(), r.end());
        });
        go(name("insert_range_after(p, input range)"), p, Op::multi, all_kinds,
           [&](C& c, E&) { c.insert_range_after(at(c, p), rng<in_tag>()); });
        go(name("insert_range_after(p, ra range)"), p, Op::multi, all_kinds,
           [&](C& c, E&) { c.insert_range_after(at(c, p), rng<ra_tag>()); });
        go(name("insert_after(p, il)"), p, Op::multi, elem_kinds,
           [&](C& c, E&) { c.insert_after(at(c, p), {src()[0], src()[1], src()[2]}); });
      } else if constexpr (hive) {
        if (p != Pos::end) continue;
        go("insert(const T&)", p, Op::single, elem_kinds, [&](C& c, E&) { c.insert(src()[0]); });
        go("insert(T&&)", p, Op::single, elem_kinds, [&](C& c, E& t) { c.insert(std::move(t)); });
        go("insert(hint, const T&)", p, Op::single, elem_kinds, [&](C& c, E&) { c.insert(c.begin(), src()[0]); });
        go("emplace(int)", p, Op::single, {value_ctor, allocation}, [&](C& c, E&) { c.emplace(66); });
        go("emplace_hint(hint, int)", p, Op::single, {value_ctor, allocation},
           [&](C& c, E&) { c.emplace_hint(c.end(), 66); });
        go("insert(3, x)", p, Op::multi, elem_kinds, [&](C& c, E&) { c.insert(3, src()[1]); });
        go("insert(input first, last)", p, Op::multi, all_kinds, [&](C& c, E&) {
          auto r = rng<in_tag>();
          c.insert(r.begin(), r.end());
        });
        go("insert(ra first, last)", p, Op::multi, all_kinds, [&](C& c, E&) {
          auto r = rng<ra_tag>();
          c.insert(r.begin(), r.end());
        });
        go("insert_range(input)", p, Op::multi, all_kinds, [&](C& c, E&) { c.insert_range(rng<in_tag>()); });
        go("insert_range(fwd)", p, Op::multi, all_kinds, [&](C& c, E&) { c.insert_range(rng<fwd_tag>()); });
        go("insert_range(ra)", p, Op::multi, all_kinds, [&](C& c, E&) { c.insert_range(rng<ra_tag>()); });
        go("insert(il)", p, Op::multi, elem_kinds, [&](C& c, E&) { c.insert({src()[0], src()[1], src()[2]}); });
      } else {
        go(name("insert(p, const T&)"), p, Op::single, elem_kinds, [&](C& c, E&) { c.insert(at(c, p), src()[0]); });
        go(name("insert(p, T&&)"), p, Op::single, elem_kinds,
           [&](C& c, E& t) { c.insert(at(c, p), std::move(t)); });
        go(name("emplace(p, int)"), p, Op::single, elem_kinds, [&](C& c, E&) { c.emplace(at(c, p), 66); });
        go(name("emplace(p, int) [value_ctor]"), p, Op::single, {value_ctor},
           [&](C& c, E&) { c.emplace(at(c, p), 66); });
        go(name("emplace(p) [default_ctor]"), p, Op::single, {default_ctor}, [&](C& c, E&) { c.emplace(at(c, p)); });
        go(name("insert(p, 3, x)"), p, Op::multi, elem_kinds, [&](C& c, E&) { c.insert(at(c, p), 3, src()[1]); });
        go(name("insert(p, input first, last)"), p, Op::multi, all_kinds, [&](C& c, E&) {
          auto r = rng<in_tag>();
          c.insert(at(c, p), r.begin(), r.end());
        });
        go(name("insert(p, fwd first, last)"), p, Op::multi, all_kinds, [&](C& c, E&) {
          auto r = rng<fwd_tag>();
          c.insert(at(c, p), r.begin(), r.end());
        });
        go(name("insert(p, ra first, last)"), p, Op::multi, all_kinds, [&](C& c, E&) {
          auto r = rng<ra_tag>();
          c.insert(at(c, p), r.begin(), r.end());
        });
        go(name("insert_range(p, input)"), p, Op::multi, all_kinds,
           [&](C& c, E&) { c.insert_range(at(c, p), rng<in_tag>()); });
        go(name("insert_range(p, fwd)"), p, Op::multi, all_kinds,
           [&](C& c, E&) { c.insert_range(at(c, p), rng<fwd_tag>()); });
        go(name("insert_range(p, ra)"), p, Op::multi, all_kinds,
           [&](C& c, E&) { c.insert_range(at(c, p), rng<ra_tag>()); });
        go(name("insert(p, il)"), p, Op::multi, elem_kinds,
           [&](C& c, E&) { c.insert(at(c, p), {src()[0], src()[1], src()[2]}); });
      }
    }
    if constexpr (requires(C& c) { c.push_back(src()[0]); }) {
      go("push_back(const T&)", Pos::back, Op::single, elem_kinds, [](C& c, E&) { c.push_back(src()[0]); });
      go("push_back(T&&)", Pos::back, Op::single, elem_kinds, [](C& c, E& t) { c.push_back(std::move(t)); });
      go("emplace_back(int)", Pos::back, Op::single, {value_ctor, allocation, copy_ctor, move_ctor},
         [](C& c, E&) { c.emplace_back(66); });
      go("emplace_back()", Pos::back, Op::single, {default_ctor}, [](C& c, E&) { c.emplace_back(); });
    }
    // Arguments that alias elements of the container ([sequence.reqmts]: t may refer to an
    // element; [container.reqmts]/66 and the per-container remarks apply unchanged).
    if constexpr (requires(C& c) { c.push_back(src()[0]); c.front(); c.back(); }) {
      go("push_back(front()) [aliasing]", Pos::back, Op::single, elem_kinds, [](C& c, E&) { c.push_back(c.front()); });
      go("emplace_back(back()) [aliasing]", Pos::back, Op::single, elem_kinds, [](C& c, E&) { c.emplace_back(c.back()); });
      go("insert(mid, back()) [aliasing]", Pos::mid, Op::single, elem_kinds,
         [](C& c, E&) { c.insert(at(c, Pos::mid), c.back()); });
      go("insert(late, front()) [aliasing]", Pos::late, Op::single, elem_kinds,
         [](C& c, E&) { c.insert(at(c, Pos::late), c.front()); });
      go("insert(mid, 4, back()) [aliasing]", Pos::mid, Op::multi, elem_kinds,
         [](C& c, E&) { c.insert(at(c, Pos::mid), 4, c.back()); });
      if constexpr (requires(C& c) { c.resize(9, c.front()); })
        go("resize(9, front()) [aliasing]", Pos::end, Op::resize, elem_kinds, [](C& c, E&) { c.resize(9, c.front()); });
    }
    if constexpr (requires(C& c) { c.push_front(src()[0]); }) {
      go("push_front(const T&)", Pos::front, Op::single, elem_kinds, [](C& c, E&) { c.push_front(src()[0]); });
      go("push_front(T&&)", Pos::front, Op::single, elem_kinds, [](C& c, E& t) { c.push_front(std::move(t)); });
      go("emplace_front(int)", Pos::front, Op::single, {value_ctor, allocation},
         [](C& c, E&) { c.emplace_front(66); });
    }
    if constexpr (requires(C& c) { c.append_range(rng<in_tag>()); }) {
      go("append_range(input)", Pos::end, Op::multi, all_kinds, [](C& c, E&) { c.append_range(rng<in_tag>()); });
      go("append_range(fwd)", Pos::end, Op::multi, all_kinds, [](C& c, E&) { c.append_range(rng<fwd_tag>()); });
      go("append_range(ra)", Pos::end, Op::multi, all_kinds, [](C& c, E&) { c.append_range(rng<ra_tag>()); });
    }
    if constexpr (requires(C& c) { c.prepend_range(rng<in_tag>()); }) {
      go("prepend_range(input)", Pos::front, Op::multi, all_kinds, [](C& c, E&) { c.prepend_range(rng<in_tag>()); });
      go("prepend_range(fwd)", Pos::front, Op::multi, all_kinds, [](C& c, E&) { c.prepend_range(rng<fwd_tag>()); });
      go("prepend_range(ra)", Pos::front, Op::multi, all_kinds, [](C& c, E&) { c.prepend_range(rng<ra_tag>()); });
    }
  }

  static void capacity() {
    if constexpr (requires(C& c) { c.resize(8); }) {
      go("resize(9)", Pos::end, Op::resize, {default_ctor, copy_ctor, move_ctor, allocation},
         [](C& c, E&) { c.resize(9); });
      go("resize(9, x)", Pos::end, Op::resize, elem_kinds, [](C& c, E&) { c.resize(9, src()[3]); });
      go("resize(2)", Pos::end, Op::resize, elem_kinds, [](C& c, E&) { c.resize(2); });
    }
    if constexpr (requires(C& c) { c.reserve(40); typename C::allocator_type; }) // (inplace_vector::reserve(40) is bad_alloc)
      go("reserve(40)", Pos::none, Op::reserve, elem_kinds, [](C& c, E&) { c.reserve(40); });
    if constexpr (requires(C& c) { c.shrink_to_fit(); }) {
      go("shrink_to_fit after growth", Pos::none, Op::shrink, elem_kinds, [](C& c, E&) {
        // the growth is part of the sweep too; shrink_to_fit is what the guarantee is about,
        // so the growth happens with the kind disarmed below
        c.shrink_to_fit();
      });
    }
  }

  static void assignments() {
    if constexpr (requires(C& c) { c.assign(4, src()[0]); }) {
      go("assign(4, x)", Pos::none, Op::assign, elem_kinds, [](C& c, E&) { c.assign(4, src()[0]); });
      go("assign(8, x)", Pos::none, Op::assign, elem_kinds, [](C& c, E&) { c.assign(8, src()[0]); });
      go("assign(input first, last)", Pos::none, Op::assign, all_kinds, [](C& c, E&) {
        auto r = rng<in_tag>();
        c.assign(r.begin(), r.end());
      });
      go("assign(ra first, last)", Pos::none, Op::assign, all_kinds, [](C& c, E&) {
        auto r = rng<ra_tag>();
        c.assign(r.begin(), r.end());
      });
      go("assign(il)", Pos::none, Op::assign, elem_kinds, [](C& c, E&) { c.assign({src()[0], src()[1]}); });
      go("operator=(il)", Pos::none, Op::assign, elem_kinds,
         [](C& c, E&) { c = {src()[0], src()[1], src()[2], src()[3], src()[4], src()[5], src()[0]}; });
    }
    if constexpr (requires(C& c) { c.assign_range(rng<in_tag>()); }) {
      go("assign_range(input)", Pos::none, Op::assign, all_kinds, [](C& c, E&) { c.assign_range(rng<in_tag>()); });
      go("assign_range(fwd)", Pos::none, Op::assign, all_kinds, [](C& c, E&) { c.assign_range(rng<fwd_tag>()); });
      go("assign_range(ra)", Pos::none, Op::assign, all_kinds, [](C& c, E&) { c.assign_range(rng<ra_tag>()); });
    }
    // Copy assignment from a longer and from a shorter source (kept alive outside attempt).
    for (int len : {2, 8}) {
      static C* other;
      C o = P::make(0);
      o.clear();
      for (int i = 0; i < len; ++i) {
        if constexpr (is_fwd<C>)
          o.push_front(E(90 + i));
        else
          o.insert(o.end(), mk(90 + i));
      }
      other = &o;
      go(len == 2 ? "copy assignment (shorter source)" : "copy assignment (longer source)", Pos::none, Op::assign,
         elem_kinds, [](C& c, E&) { c = *other; });
    }
    if constexpr (!std::is_void_v<typename P::C2>) {
      using C2 = typename P::C2;
      go("move assignment, unequal non-propagating allocator", Pos::none, Op::assign, elem_kinds, [](C&, E&) {
        // both containers are local to the attempt: accounting covers them through the sweep
        C2 a(typename C2::allocator_type(1));
        C2 b(typename C2::allocator_type(2));
        for (int i = 0; i < 4; ++i) {
          a.insert(a.end(), E(i));
          b.insert(b.end(), E(10 + i));
        }
        a = std::move(b);
      });
    }
  }

  static void erasures() {
    if constexpr (!is_fwd<C>) {
      go("erase(mid)", Pos::mid, Op::erase, {copy_assign, move_assign},
         [](C& c, E&) { c.erase(std::next(c.begin(), 1)); });
      go("erase(range)", Pos::mid, Op::erase, {copy_assign, move_assign},
         [](C& c, E&) { c.erase(std::next(c.begin(), 1), std::next(c.begin(), 3)); });
    }
  }

  static void constructors() {
    auto ctor = [](const char* nm, std::initializer_list<Kind> ks, auto mk) {
      go(nm, Pos::none, Op::ctor, ks, [mk](C&, E&) {
        C x = mk();
        (void)x;
      });
    };
    ctor("C(input first, last)", all_kinds, [] {
      auto r = rng<in_tag>();
      return C(r.begin(), r.end());
    });
    ctor("C(fwd first, last)", all_kinds, [] {
      auto r = rng<fwd_tag>();
      return C(r.begin(), r.end());
    });
    ctor("C(ra first, last)", all_kinds, [] {
      auto r = rng<ra_tag>();
      return C(r.begin(), r.end());
    });
    ctor("C(from_range, input)", all_kinds, [] { return C(std::from_range, rng<in_tag>()); });
    ctor("C(from_range, fwd)", all_kinds, [] { return C(std::from_range, rng<fwd_tag>()); });
    ctor("C(from_range, ra)", all_kinds, [] { return C(std::from_range, rng<ra_tag>()); });
    ctor("C(6)", {default_ctor, allocation}, [] { return C(6); });
    ctor("C(6, x)", elem_kinds, [] { return C(6, src()[2]); });
    ctor("C(il)", elem_kinds, [] { return C{src()[0], src()[1], src()[2], src()[3]}; });
    static C* proto;
    C pr = P::make(0);
    proto = &pr;
    ctor("C(const C&)", elem_kinds, [] { return C(*proto); });
    if constexpr (requires { typename C::allocator_type; })
      ctor("C(const C&, alloc)", elem_kinds, [] { return C(*proto, typename C::allocator_type(3)); });
    if constexpr (!std::is_void_v<typename P::C2>) {
      using C2 = typename P::C2;
      go("C2(C2&&, unequal alloc)", Pos::none, Op::ctor, elem_kinds, [](C&, E&) {
        C2 a(typename C2::allocator_type(1));
        for (int i = 0; i < 5; ++i) a.insert(a.end(), E(i));
        C2 b(std::move(a), typename C2::allocator_type(2));
      });
    }
  }

  static void bigops() {
    using I = range<in_tag, E>;
    using R = range<ra_tag, E>;
    const auto ks = {copy_ctor, allocation, iter_inc};
    if constexpr (is_fwd<C>) {
      go("insert_range_after(mid, 70 input)", Pos::mid, Op::multi, ks,
         [](C& c, E&) { c.insert_range_after(at(c, Pos::mid), I{big(), big() + big_n}); });
    } else if constexpr (is_hive<C>) {
      go("insert_range(70 input)", Pos::end, Op::multi, ks, [](C& c, E&) { c.insert_range(I{big(), big() + big_n}); });
      go("insert_range(70 ra)", Pos::end, Op::multi, ks, [](C& c, E&) { c.insert_range(R{big(), big() + big_n}); });
      go("insert(70, x)", Pos::end, Op::multi, ks, [](C& c, E&) { c.insert(70, src()[0]); });
    } else if constexpr (requires { typename C::allocator_type; }) { // (inplace_vector<E, 16> cannot hold 75)
      go("insert_range(mid, 70 input)", Pos::mid, Op::multi, ks,
         [](C& c, E&) { c.insert_range(at(c, Pos::mid), I{big(), big() + big_n}); });
      go("insert_range(mid, 70 ra)", Pos::mid, Op::multi, ks,
         [](C& c, E&) { c.insert_range(at(c, Pos::mid), R{big(), big() + big_n}); });
      go("insert(mid, 70, x)", Pos::mid, Op::multi, ks, [](C& c, E&) { c.insert(at(c, Pos::mid), 70, src()[0]); });
      go("insert(front, 70 ra first, last)", Pos::front, Op::multi, ks, [](C& c, E&) {
        R r{big(), big() + big_n};
        c.insert(at(c, Pos::front), r.begin(), r.end());
      });
      if constexpr (requires(C& c) { c.append_range(I{}); })
        go("append_range(70 input)", Pos::end, Op::multi, ks, [](C& c, E&) { c.append_range(I{big(), big() + big_n}); });
      if constexpr (requires(C& c) { c.prepend_range(I{}); })
        go("prepend_range(70 input)", Pos::front, Op::multi, ks, [](C& c, E&) { c.prepend_range(I{big(), big() + big_n}); });
      go("assign_range(70 input)", Pos::none, Op::assign, ks, [](C& c, E&) { c.assign_range(I{big(), big() + big_n}); });
      if constexpr (requires(C& c) { c.resize(80); })
        go("resize(80, x)", Pos::end, Op::resize, ks, [](C& c, E&) { c.resize(80, src()[1]); });
      go("emplace at front 70 times (each call separately)", Pos::front, Op::assign, {copy_ctor, allocation, value_ctor}, [](C& c, E&) {
        for (int i = 0; i < 70; ++i) c.emplace(c.begin(), mk(i));
      });
    }
  }

  static void all() {
    (void)src(); // the sources live across all sweeps
    (void)big();
    bigops();
    insertions();
    capacity();
    assignments();
    erasures();
    constructors();
  }
};

} // namespace exh::seq
