// [alg.move]: move and move_backward perform "*(result + n) = std::move(*(first + n))"
// (respectively "*(result - n) = std::move(*(last - n))"), and ranges::move uses
// ranges::iter_move; [uninitialized.move]: uninitialized_move(_n) is "::new (voidify(*result))
// iter_value_t<NoThrowForwardIterator>(deref-move(first))". For an rvalue S, overload
// resolution prefers a template taking U&& (constrained to U = S) over the defaulted copy
// operations (S has no move constructor or move assignment: the user-declared copy operations
// suppress them), so the templates must run even though S is trivially copyable. They mark the
// value (+2000), so a byte copy is visible.
#include <algorithm>
#include <concepts>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct S {
  int v;
  S(int x = 0) : v(x) {}
  S(const S&) = default;
  S& operator=(const S&) = default;
  template <class U>
    requires std::same_as<U, S>
  S(U&& o) : v(o.v + 2000) {}
  template <class U>
    requires std::same_as<U, S>
  S& operator=(U&& o) {
    v = o.v + 2000;
    return *this;
  }
};
static_assert(std::is_trivially_copyable_v<S>);

constexpr int N = 33;

static void fill_src(S* s) {
  for (int i = 0; i < N; ++i) s[i].v = i;
}
static void expect(const S* d, int n, int add) {
  for (int i = 0; i < n; ++i) CHECK(d[i].v == i + add);
}

int main() {
  S src[N], dst[N];
  for (int n : {0, 1, 5, 16, N}) {
    fill_src(src);
    std::move(src, src + n, dst);
    expect(dst, n, 2000);
    std::move_backward(src, src + n, dst + n);
    expect(dst, n, 2000);
    std::ranges::move(src, src + n, dst);
    expect(dst, n, 2000);
    std::ranges::move_backward(src, src + n, dst + n);
    expect(dst, n, 2000);
    std::copy(std::make_move_iterator(src), std::make_move_iterator(src + n), dst);
    expect(dst, n, 2000);
    alignas(S) unsigned char raw[sizeof(S) * N];
    S* r = reinterpret_cast<S*>(raw);
    std::uninitialized_move(src, src + n, r);
    expect(std::launder(r), n, 2000);
    std::destroy(r, r + n);
    std::uninitialized_move_n(src, n, r);
    expect(std::launder(r), n, 2000);
    std::destroy(r, r + n);
    std::ranges::uninitialized_move(src, src + n, r, r + n);
    expect(std::launder(r), n, 2000);
    std::destroy(r, r + n);
    std::uninitialized_copy(std::make_move_iterator(src), std::make_move_iterator(src + n), r);
    expect(std::launder(r), n, 2000);
    std::destroy(r, r + n);
    expect(src, n, 0);  // the sources are left as they were (the templates do not modify them)
  }
}
