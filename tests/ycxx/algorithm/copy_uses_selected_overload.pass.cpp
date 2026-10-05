// The copy algorithms are specified by the expressions they evaluate: [alg.copy] copy, copy_n,
// copy_if, copy_backward perform "*(result + n) = *(first + n)" (respectively "*(result - n) =
// *(last - n)"), and [uninitialized.copy] uninitialized_copy(_n) is "for (; first != last;
// ++result, (void)++first) ::new (voidify(*result)) iter_value_t<NoThrowForwardIterator>(*first);"
// (ranges forms likewise). For a non-const lvalue *first, overload resolution prefers a
// constructor or assignment template taking U& (U = S) over the defaulted copy operations, so
// those templates must run even though the type is trivially copyable (a template is never a
// copy constructor or copy assignment operator, [class.copy.ctor]/1, [class.copy.assign]/1).
// The templates mark the value (+1000), so a byte copy is visible. Through const iterators the
// defaulted (trivial) operations are selected and the values are copied unchanged.
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include "check.hpp"

static int ctor_hits = 0, assign_hits = 0;

struct S {
  int v;
  S(int x = 0) : v(x) {}
  S(const S&) = default;
  S& operator=(const S&) = default;
  template <class U>
    requires std::same_as<U, S>
  S(U& o) : v(o.v + 1000) {
    ++ctor_hits;
  }
  template <class U>
    requires std::same_as<U, S>
  S& operator=(U& o) {
    v = o.v + 1000;
    ++assign_hits;
    return *this;
  }
};
static_assert(std::is_trivially_copyable_v<S>);

constexpr int N = 40;

static void fill_src(S* s) {
  for (int i = 0; i < N; ++i) s[i].v = i;
}
static void expect(const S* d, int n, int add) {
  for (int i = 0; i < n; ++i) CHECK(d[i].v == i + add);
}

int main() {
  S src[N], dst[N];
  for (int n : {0, 1, 7, 16, N}) {
    fill_src(src);
    assign_hits = 0;
    std::copy(src, src + n, dst);
    expect(dst, n, 1000);
    CHECK(assign_hits == n);

    std::copy_n(src, n, dst);
    expect(dst, n, 1000);
    std::copy_backward(src, src + n, dst + n);
    expect(dst, n, 1000);
    std::copy_if(src, src + n, dst, [](const S&) { return true; });
    expect(dst, n, 1000);
    std::ranges::copy(src, src + n, dst);
    expect(dst, n, 1000);
    std::ranges::copy_backward(src, src + n, dst + n);
    expect(dst, n, 1000);
    std::ranges::copy_n(src, n, dst);
    expect(dst, n, 1000);
    CHECK(assign_hits == 7 * n);

    // through const iterators: the defaulted assignment, values unchanged
    const S* csrc = src;
    std::copy(csrc, csrc + n, dst);
    expect(dst, n, 0);
    std::ranges::copy(csrc, csrc + n, dst);
    expect(dst, n, 0);
    CHECK(assign_hits == 7 * n);

    // uninitialized_copy constructs from *first
    alignas(S) unsigned char raw[sizeof(S) * N];
    S* r = reinterpret_cast<S*>(raw);
    ctor_hits = 0;
    S* e = std::uninitialized_copy(src, src + n, r);
    CHECK(e == r + n);
    expect(std::launder(r), n, 1000);
    std::destroy(r, r + n);
    std::uninitialized_copy_n(src, n, r);
    expect(std::launder(r), n, 1000);
    std::destroy(r, r + n);
    std::ranges::uninitialized_copy(src, src + n, r, r + n);
    expect(std::launder(r), n, 1000);
    std::destroy(r, r + n);
    std::ranges::uninitialized_copy_n(src, n, r, r + n);
    expect(std::launder(r), n, 1000);
    std::destroy(r, r + n);
    CHECK(ctor_hits == 4 * n);
    std::uninitialized_copy(csrc, csrc + n, r);
    expect(std::launder(r), n, 0);
    std::destroy(r, r + n);
    CHECK(ctor_hits == 4 * n);
  }
}
