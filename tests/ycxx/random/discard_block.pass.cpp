// [rand.adapt.disc]: "If n >= r, advance the state of e from e_i to e_{i+p-r} and set n to 0. In
// any case, then increment n and advance e's then-current state e_j to e_{j+1}." The generation
// algorithm yields the value returned by that last e(). /6: each constructor that is not a copy
// constructor sets n to 0. [rand.req.adapt]: A(), A(s), A(q) initialize the base engine likewise;
// base() returns a const reference to it.
#include <random>
#include <cstdint>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "random_support.hpp"

template <class Engine, std::size_t p, std::size_t r>
struct ref_disc {
  Engine e;
  std::size_t n = 0;
  typename Engine::result_type operator()() {
    if (n >= r) {
      e.discard(p - r);
      n = 0;
    }
    ++n;
    return e();
  }
};

template <class Engine, std::size_t p, std::size_t r>
void compare(const Engine& base, int count) {
  std::discard_block_engine<Engine, p, r> a(base);
  ref_disc<Engine, p, r> ref{base};
  CHECK(a.base() == base);
  for (int i = 0; i < count; ++i) CHECK(a() == ref());
  CHECK(a.base() == ref.e);
}

using D = std::discard_block_engine<std::minstd_rand, 5, 2>;
static_assert(D::block_size == 5 && D::used_block == 2);
static_assert(std::is_same_v<decltype(D::block_size), const std::size_t>);
static_assert(std::is_same_v<D::result_type, std::minstd_rand::result_type>);
static_assert(D::min() == std::minstd_rand::min() && D::max() == std::minstd_rand::max());
static_assert(std::is_same_v<decltype(std::declval<const D&>().base()), const std::minstd_rand&>);
static_assert(noexcept(std::declval<const D&>().base()));

int main() {
  compare<std::minstd_rand, 5, 2>(std::minstd_rand(), 1000);
  compare<std::minstd_rand, 5, 2>(std::minstd_rand(17), 1000);
  compare<std::minstd_rand, 3, 3>(std::minstd_rand(), 100);  // p == r: nothing discarded
  compare<std::minstd_rand, 7, 1>(std::minstd_rand(), 300);
  compare<std::ranlux24_base, 223, 23>(std::ranlux24_base(), 1000);
  compare<std::mt19937_64, 10, 9>(std::mt19937_64(3), 500);

  // p == r gives the base engine's sequence.
  std::discard_block_engine<std::mt19937, 4, 4> same;
  std::mt19937 m;
  for (int i = 0; i < 50; ++i) CHECK(same() == m());

  // Constructors.
  D d0;
  CHECK(d0.base() == std::minstd_rand());
  D d1(42);
  CHECK(d1.base() == std::minstd_rand(42));
  std::minstd_rand b(9);
  b();
  D d2(b);
  CHECK(d2.base() == b);
  std::minstd_rand bm = b;
  D d3(std::move(bm));
  CHECK(d3.base() == b);
  CHECK(d2 == d3);
  rs::pattern_seq q1, q2;
  D d4(q1);
  CHECK(d4.base() == std::minstd_rand(q2));

  // A copy keeps n: it continues identically even mid-block.
  D x(5);
  x();
  D y(x);
  for (int i = 0; i < 20; ++i) CHECK(x() == y());
  y = D(7);
  CHECK(y == D(7));

  // seed functions ([rand.req.eng]: e.seed() gives e == E(), e.seed(s) gives e == E(s)).
  D s;
  s();
  s.seed();
  CHECK(s == D());
  D fresh;
  for (int i = 0; i < 20; ++i) CHECK(s() == fresh());
  s();
  s.seed(42);
  CHECK(s == D(42));
  D fresh42(42);
  for (int i = 0; i < 20; ++i) CHECK(s() == fresh42());
  rs::pattern_seq q3, q4;
  s();
  s.seed(q3);
  CHECK(s == D(q4));

  // discard(z) is equivalent to z calls.
  D u, v;
  u.discard(37);
  for (int i = 0; i < 37; ++i) v();
  CHECK(u == v);
  CHECK(u() == v());
  // Equality depends on the block position too.
  D w1, w2;
  w1();
  CHECK(w1 != w2);
}
