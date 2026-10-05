// Allocator propagation under exceptions: copy assignment, move assignment and swap of
// allocator-aware containers whose allocators compare unequal, with propagate_on_container_*
// true and false, while element construction, the allocator, the hash or the comparison throw
// at their k-th call. Besides the harness's accounting (every element destroyed exactly once,
// every block deallocated exactly once with its n), every deallocation must be made by an
// allocator that compares equal to the one that allocated the block:
//   [allocator.requirements.general]: a.deallocate(p, n): "Preconditions: p is a value returned
//     by an earlier call to allocate that has not been invalidated by an intervening call to
//     deallocate. n matches the value passed to allocate to obtain this memory." and allocators
//     a1 == a2 iff "storage allocated from each can be deallocated via the other".
//   [container.alloc.reqmts]/(t = u, POCCA true): "the allocator ... is replaced"; the memory
//     obtained with the old allocator must still be released through an allocator equal to it,
//     also when the assignment exits via an exception (the container's state is then valid but
//     unspecified: [res.on.exception.handling]/1, basic guarantee).
//   Containers: vector, vector<bool>, deque, list, forward_list, hive, set, multimap,
//   unordered_set, unordered_multimap, basic_string.
// REQUIRES: exceptions
#include <deque>
#include <forward_list>
#if __has_include(<hive>) // (libstdc++ 16 has no <hive>: the reference run covers the others)
#include <hive>
#endif
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "exc_harness.hpp"

using namespace exh;

template <class C>
constexpr bool is_map = requires { typename C::mapped_type; };

template <class C>
typename C::value_type val(int k) {
  if constexpr (is_map<C>)
    return {T(k), T(k + 1)};
  else if constexpr (std::is_same_v<typename C::value_type, bool>)
    return k % 3 == 0;
  else if constexpr (std::is_same_v<typename C::value_type, char>)
    return char('a' + k % 26);
  else
    return T(k);
}

template <class C>
C make(int id, int n, int base) {
  C c{typename C::allocator_type(id)};
  if constexpr (std::is_same_v<typename C::value_type, char>) n *= 5; // beyond any small buffer
  for (int i = 0; i < n; ++i) {
    if constexpr (requires { c.push_front(val<C>(0)); } && !requires { c.push_back(val<C>(0)); })
      c.push_front(val<C>(base + i));
    else if constexpr (requires { c.push_back(val<C>(0)); })
      c.push_back(val<C>(base + i));
    else
      c.insert(val<C>(base + i));
  }
  return c;
}

template <class C>
long objs(const C& c) {
  long n = 0;
  for (auto it = c.begin(); it != c.end(); ++it) ++n;
  if constexpr (is_map<C>)
    return 2 * n;
  else if constexpr (std::is_same_v<typename C::value_type, T>)
    return n;
  else
    return 0;
}

template <class C>
void run(const char* cname) {
  using A = typename C::allocator_type;
  static char l1[128], l2[128], l3[128], l4[128];
  constexpr bool P = A::propagate_on_container_copy_assignment::value;
  __builtin_snprintf(l1, sizeof l1, "%s: copy assignment, unequal allocators, propagate=%d", cname, int(P));
  __builtin_snprintf(l2, sizeof l2, "%s: move assignment, unequal allocators, propagate=%d", cname, int(P));
  __builtin_snprintf(l3, sizeof l3, "%s: swap, unequal propagating allocators", cname);
  __builtin_snprintf(l4, sizeof l4, "%s: C(const C&, alloc), C(C&&, alloc), unequal", cname);
  const auto K = {copy_ctor, move_ctor, copy_assign, move_assign, allocation, hash, compare};
  for (int na : {3, 12})
    for (int nb : {2, 9})
      sweep_kinds(l1, K, [&] {
        C a = make<C>(1, na, 10);
        C b = make<C>(2, nb, 50);
        long live0 = st.live, oa = objs(a), ob = objs(b);
        bool threw = attempt([&] { a = b; });
        EXH_EXPECT(st.live - live0 == objs(a) - oa + objs(b) - ob, "element accounting");
        if (!threw) {
          EXH_EXPECT(a.get_allocator().id == (P ? 2 : 1), "allocator after copy assignment");
          EXH_EXPECT(snap::of(a) == snap::of(b), "copy assignment result");
        }
        return threw;
      });
  for (int na : {3, 12})
    sweep_kinds(l2, K, [&] {
      C a = make<C>(1, na, 10);
      C b = make<C>(2, 7, 50);
      snap sb = snap::of(b);
      bool threw = attempt([&] { a = std::move(b); });
      if (!threw) {
        EXH_EXPECT(a.get_allocator().id == (P ? 2 : 1), "allocator after move assignment");
        EXH_EXPECT(snap::of(a) == sb, "move assignment result");
      }
      return threw;
    });
  if constexpr (P)
    sweep_kinds(l3, K, [&] {
      C a = make<C>(1, 4, 10);
      C b = make<C>(2, 7, 50);
      bool threw = attempt([&] { a.swap(b); });
      EXH_EXPECT(!threw, "swap threw");
      EXH_EXPECT(a.get_allocator().id == 2 && b.get_allocator().id == 1, "allocators not swapped");
      return threw;
    });
  sweep_kinds(l4, K, [&] {
    C b = make<C>(2, 7, 50);
    return attempt([&] {
      C c(b, A(3));
      C d(std::move(b), A(4));
      EXH_EXPECT(c.get_allocator().id == 3 && d.get_allocator().id == 4, "allocator-extended constructors");
    });
  });
}

int main() {
  run<std::vector<T, alloc<T, true>>>("vector");
  run<std::vector<T, alloc<T, false>>>("vector");
  run<std::vector<bool, alloc<bool, true>>>("vector<bool>");
  run<std::vector<bool, alloc<bool, false>>>("vector<bool>");
  run<std::deque<T, alloc<T, true>>>("deque");
  run<std::deque<T, alloc<T, false>>>("deque");
  run<std::list<T, alloc<T, true>>>("list");
  run<std::list<T, alloc<T, false>>>("list");
  run<std::forward_list<T, alloc<T, true>>>("forward_list");
  run<std::forward_list<T, alloc<T, false>>>("forward_list");
#if __has_include(<hive>)
  run<std::hive<T, alloc<T, true>>>("hive");
  run<std::hive<T, alloc<T, false>>>("hive");
#endif
  run<std::set<T, exh::less, alloc<T, true>>>("set");
  run<std::set<T, exh::less, alloc<T, false>>>("set");
  run<std::multimap<T, T, exh::less, alloc<std::pair<const T, T>, true>>>("multimap");
  run<std::multimap<T, T, exh::less, alloc<std::pair<const T, T>, false>>>("multimap");
  run<std::unordered_set<T, hasher, equal, alloc<T, true>>>("unordered_set");
  run<std::unordered_set<T, hasher, equal, alloc<T, false>>>("unordered_set");
  run<std::unordered_multimap<T, T, hasher, equal, alloc<std::pair<const T, T>, true>>>("unordered_multimap");
  run<std::unordered_multimap<T, T, hasher, equal, alloc<std::pair<const T, T>, false>>>("unordered_multimap");
  run<std::basic_string<char, std::char_traits<char>, alloc<char, true>>>("basic_string");
  run<std::basic_string<char, std::char_traits<char>, alloc<char, false>>>("basic_string");
  return finish();
}
