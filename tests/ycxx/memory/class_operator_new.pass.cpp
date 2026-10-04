// Which allocation functions are used for a class with its own operator new/delete:
// [unique.ptr.create]/2: make_unique<T>(args) "Returns: unique_ptr<T>(new T(std::forward<
//   Args>(args)...))"; /5 make_unique<T[]>(n): "unique_ptr<T>(new remove_extent_t<T>[n]())";
//   /8 make_unique_for_overwrite<T>(): "unique_ptr<T>(new T)" -- new-expressions, which find
//   the class's operator new; [unique.ptr.dltr.dflt]/3 default_delete: "calls delete on ptr"
//   (class operator delete); /3 of [unique.ptr.dltr.dflt1] delete[].
// [util.smartptr.shared.create]/7.4, /7.6, /7.8: make_shared constructs with "::new(pv) U(...)"
//   (global placement new: a deleted class placement form does not interfere) in storage it
//   allocates itself, never through the class's operator new; /7.5: allocate_shared uses the
//   allocator; [allocator.members]/5: std::allocator<T>::allocate gets storage from
//   "::operator new". The same holds for the allocator-aware containers
//   ([container.alloc.reqmts]: elements are constructed with allocator_traits::construct, which
//   for std::allocator is construct_at: "::new (voidify(*location)) T(...)",
//   [specialized.construct]). The vocabulary types and inplace_vector nest their values in
//   their own storage ([optional.optional.general]/1, [variant.variant.general]/1,
//   [expected.object.general]/1, [inplace.vector.overview]/1): no allocation function at all.
// /7.4 also says U(l...) is parenthesized: make_shared<Aggregate>(1, 2L) works
// ([dcl.init.general]/16.6.2.2) and make_shared<vector<int>>(3, 4) has three elements.
#include <memory>
#include <cstdlib>
#include <deque>
#include <expected>
#include <inplace_vector>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>
#include "check.hpp"

static int cnew = 0, cdel = 0, cnewa = 0, cdela = 0;
struct C {
  int v = 0;
  C() = default;
  C(int x) : v(x) {}
  static void* operator new(std::size_t n) {
    ++cnew;
    return std::malloc(n);
  }
  static void operator delete(void* p) {
    ++cdel;
    std::free(p);
  }
  static void* operator new[](std::size_t n) {
    ++cnewa;
    return std::malloc(n);
  }
  static void operator delete[](void* p) {
    ++cdela;
    std::free(p);
  }
};
struct CP : C {  // also hides placement new: only ::new(pv) works
  using C::C;
  static void* operator new(std::size_t, void*) = delete;
};
struct Agg {
  int a;
  long b;
};

int main() {
  {
    auto u = std::make_unique<C>(3);
    CHECK(cnew == 1 && u->v == 3);
    u.reset();
    CHECK(cdel == 1);
    auto a = std::make_unique<C[]>(4);
    CHECK(cnewa == 1);
    a.reset();
    CHECK(cdela == 1);
    auto o = std::make_unique_for_overwrite<C>();
    CHECK(cnew == 2);
  }
  CHECK(cdel == 2);
  {
    std::shared_ptr<C> s(new C(1));  // delete p
  }
  CHECK(cnew == 3 && cdel == 3);
  cnew = cdel = cnewa = cdela = 0;
  {
    auto s = std::make_shared<CP>(5);
    auto a = std::make_shared<CP[]>(3);
    auto b = std::make_shared<CP[2]>(CP(1));
    auto o = std::make_shared_for_overwrite<CP>();
    auto as = std::allocate_shared<CP>(std::allocator<CP>(), 2);
    auto aa = std::allocate_shared<CP[]>(std::allocator<CP>(), 2);
    std::optional<C> op;
    op.emplace(1);
    std::variant<int, C> va;
    va.emplace<C>(2);
    va = C(3);
    std::expected<C, int> ex(std::unexpect, 1);
    ex = C(4);
    std::vector<CP> v(10);
    v.emplace_back(1);
    std::deque<CP> d(5);
    std::list<CP> l(3);
    std::map<int, CP> m{{1, CP(1)}};
    std::unordered_map<int, CP> um;
    um[1] = CP(1);
    std::inplace_vector<C, 4> iv(2);
    CHECK(s->v == 5 && b[1].v == 1 && as->v == 2 && op->v == 1 && std::get<C>(va).v == 3 && ex->v == 4);
  }
  CHECK(cnew == 0 && cdel == 0 && cnewa == 0 && cdela == 0);
  auto ag = std::make_shared<Agg>(1, 2L);
  CHECK(ag->a == 1 && ag->b == 2);
  auto vv = std::make_shared<std::vector<int>>(3, 4);
  CHECK(vv->size() == 3 && (*vv)[2] == 4);
  auto vi = std::make_shared<int>();
  CHECK(*vi == 0);
  return 0;
}
