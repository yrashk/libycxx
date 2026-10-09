// [rand.req.urng] permits any unsigned integer result type, including extended types.
// [rand.util.canonical]/1-5 uses the full mathematical R and S and exactly k draws per
// attempt. Neither the range nor generated values may be truncated to 64 bits.
// The counter has a full cycle over [Min, Max], constant-time draws, and equal marginal
// frequency for each result. Specified arithmetic is checked independently of sample moments.
#include <random>
#include <limits>
#ifdef __STDCPP_FLOAT128_T__
#include <stdfloat>
#endif
#include "check.hpp"

using UInt = unsigned __int128;

template<UInt Min, UInt Max>
struct Counter {
  using result_type = UInt;
  UInt state;
  unsigned calls = 0;
  static constexpr UInt min() { return Min; }
  static constexpr UInt max() { return Max; }
  UInt operator()() {
    ++calls;
    UInt value = state;
    state = state == Max ? Min : state + 1;
    return value;
  }
};

static_assert(std::uniform_random_bit_generator<Counter<0, ~UInt(0)>>);

template<class Real>
void check() {
  {
    // R=2^65, S=2^64: k=1, x=2^(65-d), result=1/2.
    Counter<0, (UInt(1) << 65) - 1> g{UInt(1) << 64};
    CHECK((std::generate_canonical<Real, 24>(g) == Real(0.5)));
    CHECK(g.calls == 1);
  }
  {
    // Full 128-bit range: R=2^128 (one beyond the largest result_type value).
    Counter<0, ~UInt(0)> g{UInt(1) << 127};
    CHECK((std::generate_canonical<Real, 24>(g) == Real(0.5)));
    CHECK(g.calls == 1);
    CHECK((std::generate_canonical<Real, 0>(g) == Real(0)));
    CHECK(g.calls == 1);
  }
  {
    // A nonzero high-bit minimum must be subtracted before accumulating S.
    constexpr UInt min = UInt(1) << 100;
    Counter<min, min + (UInt(1) << 65) - 1> g{min + (UInt(1) << 64)};
    CHECK((std::generate_canonical<Real, 24>(g) == Real(0.5)));
    CHECK(g.calls == 1);
  }
  {
    // R=2^65+1, d=2, x=2^63, limit=2^65. Bucket boundaries are exact.
    constexpr UInt quarter = UInt(1) << 63;
    for (unsigned bucket = 0; bucket < 4; ++bucket) {
      Counter<0, UInt(1) << 65> g{UInt(bucket) * quarter};
      CHECK((std::generate_canonical<Real, 2>(g) == Real(bucket) / Real(4)));
      CHECK(g.calls == 1);
    }
    Counter<0, UInt(1) << 65> last{(UInt(1) << 65) - 1};
    CHECK((std::generate_canonical<Real, 2>(last) == Real(0.75)));
    Counter<0, UInt(1) << 65> rejected{UInt(1) << 65};
    CHECK((std::generate_canonical<Real, 2>(rejected) == Real(0)));
    CHECK(rejected.calls == 2);  // S==limit is rejected; next counter value is zero
  }
}

#ifdef __STDCPP_FLOAT128_T__
void check_binary128() {
  // R=2^112+1, d=113, k=2: R^2 occupies 225 bits.
  // x=floor(R^2/2^113)=2^111+1. First draws {2^112,0} give floor(S/x)=1.
  Counter<0, UInt(1) << 112> g{UInt(1) << 112};
  std::float128_t expected = 1;
  for (int i = 0; i < 113; ++i) expected /= 2;
  CHECK((std::generate_canonical<std::float128_t, 113>(g) == expected));
  CHECK(g.calls == 2);
}
#endif

int main() {
  check<float>();
  check<double>();
  check<long double>();
#ifdef __STDCPP_FLOAT128_T__
  check<std::float128_t>();
  check_binary128();
#endif
}
