// [intseq.binding] + [dcl.struct.bind]/6: tuple_size<integer_sequence<int, 1, 2>> is 2, so a
// structured binding declaration introducing three names is ill-formed.
#include <utility>

void f() {
  auto [a, b, c] = std::integer_sequence<int, 1, 2>{};
  (void)a; (void)b; (void)c;
}
