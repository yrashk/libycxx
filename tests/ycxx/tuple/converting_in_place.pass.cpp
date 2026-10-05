// tuple's and pair's converting constructors and pair's piecewise constructor initialize each
// element directly from the forwarded value or element: no temporary of the element type, no
// copy or move, and the source's value category and constness are kept.
//   [tuple.cnstr]/12: tuple(UTypes&&... u): "Initializes the elements in the tuple with the
//     corresponding value in std::forward<UTypes>(u)"; /19-21 tuple(tuple<UTypes...>& / const& /
//     && / const&& u): "initializes the ith element of *this with get<i>(FWD(u))"; /25 the pair
//     forms: "the first element with get<0>(FWD(u)) and the second element with
//     get<1>(FWD(u))"; /30 tuple(UTuple&& u) for a tuple-like UTuple: "get<i>(std::forward<
//     UTuple>(u))"; /33 the allocator_arg_t forms: the same, "except that each element of
//     non-reference type is constructed with uses-allocator construction".
//   [pairs.pair]: pair(U1&& x, U2&& y): "Initializes first with std::forward<U1>(x) and second
//     with std::forward<U2>(y)"; pair(pair<U1, U2>& / const& / && / const&& p): "Initializes
//     first with get<0>(FWD(p)) and second with get<1>(FWD(p))"; pair(P&& p) for a pair-like P:
//     the same with std::forward<P>(p); pair(piecewise_construct_t, tuple<Args1...> first_args,
//     tuple<Args2...> second_args): "Initializes first with arguments of types Args1...
//     obtained by forwarding the elements of first_args and initializes second with arguments
//     of types Args2... obtained by forwarding the elements of second_args" ("forwarding an
//     element x of type U within a tuple object means calling std::forward<U>(x)").
// The element type One can be neither copied nor moved, so any temporary-then-move would not
// compile; its constructors record how they received their Arg.
#include <array>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "inplace_probe.hpp"

using probe::Arg;
using probe::counts;
using probe::Pinned;

struct One {
  int cat = 0, v = 0;
  One(Arg& a) : cat(probe::lref), v(a.v) {}
  One(const Arg& a) : cat(probe::clref), v(a.v) {}
  One(Arg&& a) : cat(probe::rref), v(a.v) { a.moved_from = true; }
  One(const Arg&& a) : cat(probe::crref), v(a.v) {}
  One(const One&) = delete;
};

template <class A>
constexpr int cat_of() {
  if constexpr (std::is_lvalue_reference_v<A>)
    return std::is_const_v<std::remove_reference_t<A>> ? probe::clref : probe::lref;
  else
    return std::is_const_v<std::remove_reference_t<A>> ? probe::crref : probe::rref;
}

template <class T>
struct TagAlloc {
  using value_type = T;
  int id = 0;
  TagAlloc() = default;
  explicit TagAlloc(int i) : id(i) {}
  template <class U>
  TagAlloc(const TagAlloc<U>& o) noexcept : id(o.id) {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  friend bool operator==(const TagAlloc&, const TagAlloc&) = default;
};

// Uses the allocator with the leading convention; neither copyable nor movable.
struct OneA {
  using allocator_type = TagAlloc<char>;
  int cat = 0, alloc_id = -1;
  template <class A>
    requires std::is_same_v<std::remove_cvref_t<A>, Arg>
  OneA(std::allocator_arg_t, const allocator_type& al, A&&) : cat(cat_of<A&&>()), alloc_id(al.id) {}
  // The tuple constructors' constraints check is_constructible_v<OneA, Ui> (the allocator_arg_t
  // forms are "equivalent to the preceding constructors" but for uses-allocator construction),
  // so OneA is also constructible without an allocator; that constructor is never chosen here.
  template <class A>
    requires std::is_same_v<std::remove_cvref_t<A>, Arg>
  OneA(A&&) : cat(cat_of<A&&>()) {}
  OneA(const OneA&) = delete;
};

struct Lit {
  int cat;
  constexpr Lit(int&) : cat(probe::lref) {}
  constexpr Lit(const int&) : cat(probe::clref) {}
  constexpr Lit(int&&) : cat(probe::rref) {}
  Lit(const Lit&) = delete;
};

constexpr bool constant() {
  int x = 1;
  const int cx = 2;
  std::tuple<Lit, Lit, Lit> t(x, cx, 3);
  std::pair<Lit, Lit> p(x, 4);
  std::pair<Lit, Lit> q(std::piecewise_construct, std::forward_as_tuple(cx), std::forward_as_tuple(std::move(x)));
  std::tuple<int, int> src(5, 6);
  std::tuple<Lit, Lit> u(src);
  std::tuple<Lit, Lit> w(std::move(src));
  return get<0>(t).cat == probe::lref && get<1>(t).cat == probe::clref && get<2>(t).cat == probe::rref &&
         p.first.cat == probe::lref && p.second.cat == probe::rref && q.first.cat == probe::clref &&
         q.second.cat == probe::rref && get<0>(u).cat == probe::lref && get<1>(w).cat == probe::rref;
}
static_assert(constant());

void tuple_cases() {
  Arg a{1};
  const Arg ca{2};
  {
    std::tuple<One, One, One, One> t(a, ca, std::move(ca), Arg{3});
    CHECK(get<0>(t).cat == probe::lref && get<1>(t).cat == probe::clref);
    CHECK(get<2>(t).cat == probe::crref && get<3>(t).cat == probe::rref && get<3>(t).v == 3);
  }
  // From another tuple, in each of the four value categories.
  {
    std::tuple<Arg, Arg> s(Arg{4}, Arg{5});
    std::tuple<One, One> l(s);
    std::tuple<One, One> cl(std::as_const(s));
    std::tuple<One, One> cr(std::move(std::as_const(s)));
    CHECK(get<0>(l).cat == probe::lref && get<1>(cl).cat == probe::clref);
    CHECK(get<0>(cr).cat == probe::crref && !get<0>(s).moved_from);
    std::tuple<One, One> r(std::move(s));
    CHECK(get<0>(r).cat == probe::rref && get<1>(r).v == 5 && get<0>(s).moved_from && get<1>(s).moved_from);
  }
  // A tuple of references: the reference's own category (get<i> of an rvalue tuple<Arg&> is
  // Arg&).
  {
    Arg b{6};
    std::tuple<Arg&, const Arg&, Arg&&> refs(b, ca, std::move(b));
    std::tuple<One, One, One> from(std::move(refs));
    CHECK(get<0>(from).cat == probe::lref && get<1>(from).cat == probe::clref && get<2>(from).cat == probe::rref);
  }
  // From a pair, and from a tuple-like array.
  {
    std::pair<Arg, Arg> p(Arg{7}, Arg{8});
    std::tuple<One, One> l(p);
    std::tuple<One, One> c(std::as_const(p));
    CHECK(get<0>(l).cat == probe::lref && get<1>(c).cat == probe::clref);
    std::tuple<One, One> r(std::move(p));
    CHECK(get<0>(r).cat == probe::rref && p.first.moved_from && p.second.moved_from);
    std::array<Arg, 2> arr{Arg{9}, Arg{10}};
    std::tuple<One, One> al(arr);
    std::tuple<One, One> ac(std::as_const(arr));
    CHECK(get<0>(al).cat == probe::lref && get<1>(ac).cat == probe::clref && get<1>(al).v == 10);
    std::tuple<One, One> ar(std::move(arr));
    CHECK(get<0>(ar).cat == probe::rref && arr[1].moved_from);
  }
  // allocator_arg_t forms: uses-allocator construction of each element from the forwarded value.
  {
    const TagAlloc<int> al(5);
    Arg b{1};
    std::tuple<OneA, One> t(std::allocator_arg, al, b, std::move(ca));
    CHECK(get<0>(t).alloc_id == 5 && get<0>(t).cat == probe::lref && get<1>(t).cat == probe::crref);
    std::tuple<Arg, Arg> s;
    std::tuple<OneA, OneA> u(std::allocator_arg, al, std::as_const(s));
    CHECK(get<0>(u).cat == probe::clref && get<1>(u).alloc_id == 5);
    std::tuple<OneA, OneA> w(std::allocator_arg, al, std::move(s));
    CHECK(get<0>(w).cat == probe::rref && get<1>(w).cat == probe::rref);
    std::pair<Arg, Arg> p;
    std::tuple<OneA, One> x(std::allocator_arg, al, p);
    CHECK(get<0>(x).cat == probe::lref && get<0>(x).alloc_id == 5 && get<1>(x).cat == probe::lref);
  }
}

void pair_cases() {
  Arg a{1};
  const Arg ca{2};
  {
    std::pair<One, One> p(a, ca);
    CHECK(p.first.cat == probe::lref && p.second.cat == probe::clref);
    std::pair<One, One> q(std::move(ca), Arg{});
    CHECK(q.first.cat == probe::crref && q.second.cat == probe::rref);
  }
  {
    std::pair<Arg, Arg> s(Arg{3}, Arg{4});
    std::pair<One, One> l(s);
    std::pair<One, One> cl(std::as_const(s));
    std::pair<One, One> cr(std::move(std::as_const(s)));
    CHECK(l.first.cat == probe::lref && cl.second.cat == probe::clref && cr.first.cat == probe::crref);
    std::pair<One, One> r(std::move(s));
    CHECK(r.first.cat == probe::rref && r.second.v == 4 && s.first.moved_from && s.second.moved_from);
  }
  // pair-like sources: tuple and array.
  {
    std::tuple<Arg, Arg> t;
    std::pair<One, One> l(t);
    std::pair<One, One> r(std::move(t));
    CHECK(l.first.cat == probe::lref && r.second.cat == probe::rref);
    std::array<Arg, 2> arr{};
    std::pair<One, One> c(std::as_const(arr));
    CHECK(c.first.cat == probe::clref && c.second.cat == probe::clref);
  }
  // piecewise: each member from its forwarded tuple elements.
  probe::reset();
  {
    Arg b{5};
    std::pair<Pinned, Pinned> p(std::piecewise_construct, std::forward_as_tuple(1, b),
                                std::forward_as_tuple(2, std::move(b)));
    CHECK(p.first.cat == probe::lref && p.second.cat == probe::rref && b.moved_from);
    std::pair<Pinned, Pinned> q(std::piecewise_construct, std::tuple<int, const Arg&>(3, ca),
                                std::tuple<int, const Arg&&>(4, std::move(ca)));
    CHECK(q.first.cat == probe::clref && q.second.cat == probe::crref);
    std::pair<Pinned, Pinned> d(std::piecewise_construct, std::tuple<>(), std::tuple<int>(9));
    CHECK(d.first.key == 0 && d.second.key == 9);
    CHECK(counts.made == 6 && counts.extra() == 0);
  }
}

int main() {
  tuple_cases();
  pair_cases();
  return 0;
}
