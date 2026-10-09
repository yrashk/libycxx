// EXPECT-ERROR-GCC: error: 3 names provided for structured binding
// EXPECT-ERROR-GCC: note: while 'std::integer_sequence<int, 1, 2>' decomposes into 2 elements
// EXPECT-ERROR-CLANG: error: type 'std::integer_sequence<int, 1, 2>' binds to 2 elements, but 3 names were provided
// [intseq.binding] + [dcl.struct.bind]/6: tuple_size<integer_sequence<int, 1, 2>> is 2, so a
// structured binding declaration introducing three names is ill-formed.
#include <utility>

void f() {
  auto [a, b, c] = std::integer_sequence<int, 1, 2>{};
  (void)a; (void)b; (void)c;
}
