// [allocator.adaptor.members]/9-10: construct(p, args...) performs uses-allocator construction
// with inner_allocator() (uses_allocator_construction_args), through the outermost allocator's
// construct; destroy calls the outermost allocator's destroy. So nested containers receive the
// inner allocators: the second allocator for the elements, the third for their elements, and
// with fewer allocators than levels the last one repeats ([allocator.adaptor.syn]/1).
#include <scoped_allocator>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "check.hpp"
#include "test_allocators.hpp"

template <class T> using A = IdAlloc<T>;
using Inner = std::vector<int, A<int>>;
using Outer = std::vector<Inner, std::scoped_allocator_adaptor<A<Inner>, A<int>>>;

struct Tracked {
  using allocator_type = A<char>;
  int alloc_id;
  int v;
  Tracked(std::allocator_arg_t, const allocator_type& a, int x) : alloc_id(a.id), v(x) {}
};
struct TrailingAlloc {
  using allocator_type = A<char>;
  int alloc_id;
  int v;
  TrailingAlloc(int x, const allocator_type& a) : alloc_id(a.id), v(x) {}
};
struct NoAlloc {
  int v;
  explicit NoAlloc(int x) : v(x) {}
};

int main() {
  Outer o(std::scoped_allocator_adaptor<A<Inner>, A<int>>(A<Inner>(1), A<int>(2)));
  o.emplace_back(3, 7);  // Inner(3, 7, A<int>(2))
  o.emplace_back();
  CHECK(o.get_allocator().outer_allocator().id == 1);
  CHECK(o[0].get_allocator().id == 2 && o[0].size() == 3 && o[0][2] == 7);
  CHECK(o[1].get_allocator().id == 2);
  o.resize(4);
  CHECK(o[3].get_allocator().id == 2);

  // Single allocator: the adaptor itself is passed down to every level.
  using In1 = std::vector<int, std::scoped_allocator_adaptor<A<int>>>;
  using Out1 = std::vector<In1, std::scoped_allocator_adaptor<A<In1>>>;
  Out1 o1(std::scoped_allocator_adaptor<A<In1>>(A<In1>(7)));
  o1.emplace_back(2, 1);
  CHECK(o1[0].get_allocator().outer_allocator().id == 7 && o1[0].size() == 2);

  // Leading allocator_arg_t convention and trailing allocator convention.
  std::scoped_allocator_adaptor<A<Tracked>, A<char>> st(A<Tracked>(1), A<char>(9));
  Tracked* t = st.allocate(1);
  st.construct(t, 5);
  CHECK(t->alloc_id == 9 && t->v == 5);
  st.destroy(t);
  st.deallocate(t, 1);
  std::scoped_allocator_adaptor<A<TrailingAlloc>, A<char>> st2(A<TrailingAlloc>(1), A<char>(8));
  TrailingAlloc* u = st2.allocate(1);
  st2.construct(u, 6);
  CHECK(u->alloc_id == 8 && u->v == 6);
  st2.destroy(u);
  st2.deallocate(u, 1);
  // A type that does not use allocators is constructed normally.
  std::scoped_allocator_adaptor<A<NoAlloc>, A<char>> st3;
  NoAlloc* n = st3.allocate(1);
  st3.construct(n, 4);
  CHECK(n->v == 4);
  st3.destroy(n);
  st3.deallocate(n, 1);

  // Pairs: each member gets uses-allocator construction (piecewise).
  using PairT = std::pair<TrailingAlloc, Tracked>;
  std::scoped_allocator_adaptor<A<PairT>, A<char>> sp(A<PairT>(1), A<char>(3));
  PairT* pp = sp.allocate(1);
  sp.construct(pp, std::piecewise_construct, std::forward_as_tuple(1), std::forward_as_tuple(2));
  CHECK(pp->first.alloc_id == 3 && pp->second.alloc_id == 3 && pp->first.v == 1 && pp->second.v == 2);
  sp.destroy(pp);
  sp.deallocate(pp, 1);

  // Through a map: keys and values that are strings receive the inner allocator.
  using Str = std::basic_string<char, std::char_traits<char>, A<char>>;
  using MapAlloc = std::scoped_allocator_adaptor<A<std::pair<const Str, Str>>, A<char>>;
  std::map<Str, Str, std::less<Str>, MapAlloc> m(MapAlloc(A<std::pair<const Str, Str>>(1), A<char>(4)));
  m.emplace("key", "value");
  CHECK(m.begin()->first.get_allocator().id == 4 && m.begin()->second.get_allocator().id == 4);
  return 0;
}
