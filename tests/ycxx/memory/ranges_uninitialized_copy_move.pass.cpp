// [uninitialized.copy]/5: ranges::uninitialized_copy(ifirst, ilast, ofirst, olast):
// "for (; ifirst != ilast && ofirst != olast; ++ofirst, (void)++ifirst) ::new (voidify(*ofirst))
// remove_reference_t<iter_reference_t<O>>(*ifirst); return {std::move(ifirst), ofirst};"
// /10: uninitialized_copy_n(ifirst, n, ofirst, olast) via counted_iterator, returning
// {in.base(), out}. [uninitialized.move]/4,9: the same with ranges::iter_move(ifirst).
// The result types are uninitialized_copy_result<I, O> etc. (in_out_result). All constexpr.
#include <memory>
#include <span>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct M {
  int v;
  bool moved_from = false;
  constexpr M(int x) : v(x) {}
  constexpr M(M&& o) : v(o.v) { o.moved_from = true; }
  constexpr M(const M& o) : v(o.v + 1000) {}
};

static_assert(std::is_same_v<std::ranges::uninitialized_copy_result<int*, long*>,
                             std::ranges::in_out_result<int*, long*>>);
static_assert(std::is_same_v<std::ranges::uninitialized_copy_n_result<int*, long*>,
                             std::ranges::in_out_result<int*, long*>>);
static_assert(std::is_same_v<std::ranges::uninitialized_move_result<int*, long*>,
                             std::ranges::in_out_result<int*, long*>>);
static_assert(std::is_same_v<std::ranges::uninitialized_move_n_result<int*, long*>,
                             std::ranges::in_out_result<int*, long*>>);

constexpr bool test() {
  const int src[5] = {1, 2, 3, 4, 5};
  std::allocator<long> a;
  long* p = a.allocate(5);
  // output shorter than input: stops at olast
  auto r = std::ranges::uninitialized_copy(src, src + 5, p, p + 3);
  if (r.in != src + 3 || r.out != p + 3 || p[2] != 3) return false;
  std::ranges::destroy(p, p + 3);
  // input shorter than output: stops at ilast
  r = std::ranges::uninitialized_copy(src, src + 2, p, p + 5);
  if (r.in != src + 2 || r.out != p + 2 || p[1] != 2) return false;
  std::ranges::destroy(p, p + 2);
  // range overload
  auto rr = std::ranges::uninitialized_copy(std::span<const int>(src, 4), std::span<long>(p, 5));
  if (std::to_address(rr.in) != src + 4 || std::to_address(rr.out) != p + 4 || p[3] != 4) return false;
  std::ranges::destroy(p, p + 4);
  // _n
  auto rn = std::ranges::uninitialized_copy_n(src + 1, 3, p, p + 5);
  if (rn.in != src + 4 || rn.out != p + 3 || p[0] != 2) return false;
  std::ranges::destroy(p, p + 3);
  rn = std::ranges::uninitialized_copy_n(src, 5, p, p + 2);  // bounded by olast
  if (rn.in != src + 2 || rn.out != p + 2) return false;
  std::ranges::destroy(p, p + 2);
  a.deallocate(p, 5);

  M ms[3] = {1, 2, 3};
  std::allocator<M> am;
  M* q = am.allocate(3);
  auto mr = std::ranges::uninitialized_move(ms, ms + 3, q, q + 3);
  if (mr.in != ms + 3 || mr.out != q + 3) return false;
  if (q[0].v != 1 || q[2].v != 3 || !ms[0].moved_from || !ms[2].moved_from) return false;
  std::ranges::destroy(q, q + 3);
  M ms2[3] = {4, 5, 6};
  auto mr2 = std::ranges::uninitialized_move(std::span<M>(ms2), std::span<M>(q, 2));
  if (std::to_address(mr2.out) != q + 2 || std::to_address(mr2.in) != ms2 + 2) return false;
  if (ms2[2].moved_from || q[1].v != 5) return false;
  std::ranges::destroy(q, q + 2);
  M ms3[2] = {7, 8};
  auto mn = std::ranges::uninitialized_move_n(ms3, 2, q, q + 3);
  if (mn.in != ms3 + 2 || mn.out != q + 2 || q[1].v != 8 || !ms3[1].moved_from) return false;
  std::ranges::destroy(q, q + 2);
  am.deallocate(q, 3);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
