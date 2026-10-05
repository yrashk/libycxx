// [expected.un.swap]/1: member swap: "Mandates: is_swappable_v<E> is true."
// EXPECT-ERROR: static assertion failed.*std::unexpected::swap: E must be swappable
#include <expected>

struct NS {
  NS(int) {}
  NS(const NS&) = default;
  NS& operator=(const NS&) = delete;  // std::swap not viable -> not swappable
};

void f() {
  std::unexpected<NS> a(1), b(2);
  a.swap(b);
}
