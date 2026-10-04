// [obj.lifetime]/1-2: template<class T> constexpr void start_lifetime(T& r) noexcept;
// "Mandates: T is a complete type and an implicit-lifetime aggregate type." "Effects: If
// the object referenced by r is already within its lifetime, no effects. Otherwise, begins
// the lifetime of the object referenced by r." Note 1: no initialization is performed, no
// subobject has its lifetime started, and a union member becomes the active member.
// is_within_lifetime ([meta.const.eval]) is consteval, so it is used under 'if consteval'.
#include <memory>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::start_lifetime(std::declval<int(&)[2]>())), void>);
static_assert(noexcept(std::start_lifetime(std::declval<int(&)[2]>())));

struct Agg { int a; long b; };

// Storage for up to N ints whose lifetime is managed manually (the typical use case).
template <int N>
struct Buffer {
  union { int data[N]; };
  constexpr Buffer() {}
  constexpr ~Buffer() {}
};

union TwoArrays {
  int ints[2];
  long longs[2];
};

union WithAgg {
  char c;
  Agg agg;
};

constexpr bool test_activate_array() {
  Buffer<4> buf;
  std::start_lifetime(buf.data);               // data becomes the active member
#if defined(__cpp_lib_is_within_lifetime)
  if consteval {
  if (!std::is_within_lifetime(&buf.data)) return false;
  }
#endif
  std::construct_at(&buf.data[0], 10);
  std::construct_at(&buf.data[3], 13);
#if defined(__cpp_lib_is_within_lifetime)
  if consteval {
  if (!std::is_within_lifetime(&buf.data[3])) return false;
  }
#endif
  return buf.data[0] + buf.data[3] == 23;
}

constexpr bool test_switch_member() {
  TwoArrays u{.ints = {1, 2}};
  std::start_lifetime(u.longs);                // switch the active member
#if defined(__cpp_lib_is_within_lifetime)
  if consteval {
  if (!std::is_within_lifetime(&u.longs)) return false;
  if (std::is_within_lifetime(&u.ints)) return false;
  }
#endif
  std::construct_at(&u.longs[1], 7L);
  return u.longs[1] == 7;
}

constexpr bool test_already_alive() {
  TwoArrays u{.ints = {3, 4}};
  std::start_lifetime(u.ints);                 // already within lifetime: no effects
  if (u.ints[0] != 3 || u.ints[1] != 4) return false;
  Agg a{5, 6};
  std::start_lifetime(a);
  return a.a == 5 && a.b == 6;
}

constexpr bool test_aggregate_member() {
  WithAgg w{.c = 'x'};
  std::start_lifetime(w.agg);
#if defined(__cpp_lib_is_within_lifetime)
  if consteval {
  if (!std::is_within_lifetime(&w.agg)) return false;
  }
#endif
  return true;
}

static_assert(test_activate_array());
static_assert(test_switch_member());
static_assert(test_already_alive());
static_assert(test_aggregate_member());

int main() {
  CHECK(test_activate_array());
  CHECK(test_switch_member());
  CHECK(test_already_alive());
  CHECK(test_aggregate_member());
  return 0;
}
