// EXPECT-ERROR-GCC: error: no matching function for call to 'async\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'async'
// [futures.async]/2.3: "Mandates: ... is_invocable_v<decay_t<F>, decay_t<Args>...>."
#include <future>

void f(int*);
void g() {
  auto r = std::async(std::launch::deferred, f, 1.0);
}
