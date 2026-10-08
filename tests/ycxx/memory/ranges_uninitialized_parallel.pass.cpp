// The ranges:: parallel overloads of the specialized memory algorithms ([memory.syn]): they take
// an execution-policy, sized random access inputs and nothrow-sized-random-access-range /
// nothrow-random-access-iterator outputs ([special.mem.concepts]/12), and have the effects of
// the sequential overloads:
// [uninitialized.copy]/5: constructs from the input until either range ends, returns
// {ifirst + n, ofirst + n}; /10: uninitialized_copy_n likewise for n elements.
// [uninitialized.move]: the same, constructing from ranges::iter_move.
// [uninitialized.fill]: constructs copies of x, returns last (or first + n).
// [uninitialized.construct.value], [uninitialized.construct.default]: value-initialization
// (zero for int) and default-initialization.
// [specialized.destroy]: destroy(exec, r) destroys each element and returns last; destroy_n
// returns first + n.
#include <memory>
#include <cstddef>
#include <execution>
#include <new>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;

int live = 0;
struct T {
  int v;
  T() : v(-7) { ++live; }  // user-provided: default-initialization runs it
  T(int x) : v(x) { ++live; }
  T(const T& o) : v(o.v) { ++live; }
  T(T&& o) noexcept : v(o.v) { o.v = 0; ++live; }
  ~T() { --live; }
};

template <class U, std::size_t N>
struct storage {
  alignas(U) unsigned char bytes[sizeof(U) * N];
  U* p() { return reinterpret_cast<U*>(bytes); }
  std::span<U> span() { return {p(), N}; }
};

template <class Pol>
void run(Pol&& pol) {
  std::vector<T> src;
  src.reserve(5);
  for (int i = 1; i <= 5; ++i) src.emplace_back(i);
  CHECK(live == 5);
  {
    storage<T, 3> s;
    auto r = rg::uninitialized_copy(pol, src, s.span());
    CHECK(r.in == src.begin() + 3 && r.out == s.span().end() && live == 8);
    CHECK(s.p()[0].v == 1 && s.p()[2].v == 3);
    auto d = rg::destroy(pol, s.span());
    static_assert(std::is_same_v<decltype(d), std::span<T>::iterator>);
    CHECK(d == s.span().end() && live == 5);
    auto r2 = rg::uninitialized_copy_n(pol, src.begin() + 3, 2, s.p(), s.p() + 3);
    CHECK(r2.in == src.end() && r2.out == s.p() + 2 && live == 7 && s.p()[1].v == 5);
    CHECK(rg::destroy_n(pol, s.p(), 2) == s.p() + 2 && live == 5);
  }
  {
    storage<T, 8> s;
    auto r = rg::uninitialized_move(pol, src.begin(), src.end(), s.p(), s.p() + 8);
    CHECK(r.in == src.end() && r.out == s.p() + 5 && live == 10);
    CHECK(s.p()[4].v == 5 && src[4].v == 0);  // moved from
    CHECK(rg::destroy(pol, s.p(), s.p() + 5) == s.p() + 5 && live == 5);
    auto r2 = rg::uninitialized_move_n(pol, s.p(), 0, s.p() + 1, s.p() + 8);
    CHECK(r2.in == s.p() && r2.out == s.p() + 1 && live == 5);
  }
  {
    storage<T, 4> s;
    auto f = rg::uninitialized_fill(pol, s.span(), T(9));
    CHECK(f == s.span().end() && live == 9 && s.p()[3].v == 9);
    rg::destroy(pol, s.span());
    const T four(4);
    CHECK(rg::uninitialized_fill_n(pol, s.p(), 2, four) == s.p() + 2 && live == 8 && s.p()[1].v == 4);
    rg::destroy_n(pol, s.p(), 2);
    // The returned iterator is checked as a distance: GCC 16.2 at -O2 folds a returned pointer's
    // == with one past the end of a local to false (STATUS.md, known compiler bugs).
    CHECK(rg::uninitialized_default_construct(pol, s.p(), s.p() + 4) - s.p() == 4 && s.p()[2].v == -7);
    CHECK(live == 10);
    rg::destroy(pol, s.span());
    CHECK(rg::uninitialized_value_construct_n(pol, s.p(), 3) == s.p() + 3 && live == 9);
    rg::destroy_n(pol, s.p(), 3);
    CHECK(live == 6);
  }
  {
    storage<int, 4> s;
    for (int i = 0; i < 4; ++i) ::new (s.p() + i) int(55);
    CHECK(rg::uninitialized_value_construct(pol, s.span()) == s.span().end());
    CHECK(s.p()[0] == 0 && s.p()[3] == 0);
    CHECK(rg::uninitialized_default_construct_n(pol, s.p(), 4) == s.p() + 4);
  }
  {
    storage<std::string, 3> s;
    const std::vector<std::string> in{"a", "bb"};
    auto r = rg::uninitialized_copy(pol, in, s.span());
    CHECK(r.in == in.end() && r.out == s.span().begin() + 2 && s.p()[1] == "bb");
    rg::destroy(pol, s.p(), s.p() + 2);
  }
}

int main() {
  run(std::execution::seq);
  CHECK(live == 0);
  run(std::execution::par);
  run(std::execution::par_unseq);
  run(std::execution::unseq);
  CHECK(live == 0);
  return 0;
}
