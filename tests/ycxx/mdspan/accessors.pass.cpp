// [mdspan.accessor.default]: default_accessor<T>: offset_policy is itself, element_type T,
// reference T&, data_handle_type T*; access(p, i) is p[i] and offset(p, i) is p + i;
// convertible from default_accessor<U> when U(*)[] converts to T(*)[]. Trivially copyable,
// semiregular. [mdspan.accessor.aligned]: aligned_accessor<T, N>: offset_policy is
// default_accessor<T>, byte_alignment is N; convertible from aligned_accessor<U, M> for M >= N,
// explicitly constructible from default_accessor<U>, convertible to default_accessor<U>;
// access(p, i) is p[i] (assume_aligned), offset(p, i) is p + i.
#include <mdspan>
#include <concepts>
#include <type_traits>
#include "check.hpp"

using DA = std::default_accessor<int>;
static_assert(std::is_same_v<DA::offset_policy, DA> && std::is_same_v<DA::element_type, int>);
static_assert(std::is_same_v<DA::reference, int&> && std::is_same_v<DA::data_handle_type, int*>);
static_assert(std::is_trivially_copyable_v<DA> && std::semiregular<DA>);
static_assert(std::is_convertible_v<DA, std::default_accessor<const int>>);
static_assert(!std::is_constructible_v<DA, std::default_accessor<const int>>);
static_assert(!std::is_constructible_v<std::default_accessor<long>, DA>);
static_assert(std::is_nothrow_default_constructible_v<DA>);

using AA = std::aligned_accessor<double, 32>;
static_assert(AA::byte_alignment == 32);
static_assert(std::is_same_v<AA::offset_policy, std::default_accessor<double>>);
static_assert(std::is_same_v<AA::element_type, double> && std::is_same_v<AA::reference, double&>);
static_assert(std::is_same_v<AA::data_handle_type, double*>);
static_assert(std::is_trivially_copyable_v<AA> && std::semiregular<AA>);
static_assert(std::is_convertible_v<std::aligned_accessor<double, 64>, AA>);
static_assert(!std::is_constructible_v<std::aligned_accessor<double, 64>, AA>);  // weaker alignment
static_assert(std::is_convertible_v<AA, std::aligned_accessor<const double, 16>>);
static_assert(!std::is_convertible_v<std::default_accessor<double>, AA>);
static_assert(std::is_constructible_v<AA, std::default_accessor<double>>);  // explicit
static_assert(std::is_convertible_v<AA, std::default_accessor<double>>);
static_assert(std::is_convertible_v<AA, std::default_accessor<const double>>);

alignas(32) double buf[8] = {0, 1, 2, 3, 4, 5, 6, 7};

int main() {
  int x[3] = {4, 5, 6};
  DA da;
  CHECK(da.access(x, 2) == 6 && &da.access(x, 1) == &x[1] && da.offset(x, 2) == x + 2);
  static_assert(noexcept(da.access(x, 0)) && noexcept(da.offset(x, 0)));
  AA aa;
  CHECK(aa.access(buf, 3) == 3 && &aa.access(buf, 0) == buf && aa.offset(buf, 4) == buf + 4);
  static_assert(std::is_same_v<decltype(aa.offset(buf, 1)), double*>);
  std::mdspan<double, std::extents<int, 2, 4>, std::layout_right, AA> m(buf, std::layout_right::mapping<std::extents<int, 2, 4>>(), aa);
  CHECK(m[1, 3] == 7);
  std::mdspan<double, std::extents<int, 2, 4>> plain = m;  // aligned -> default accessor
  CHECK(plain[1, 0] == 4);
  return 0;
}
