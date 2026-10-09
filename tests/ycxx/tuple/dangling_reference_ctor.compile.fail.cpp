// EXPECT-ERROR-GCC: error: use of deleted function [^\n]*std::tuple[^\n]*::tuple\(
// EXPECT-ERROR-CLANG: error: call to deleted constructor of [^\n]*std::tuple
// [tuple.cnstr]/15: "This constructor is defined as deleted if
// (reference_constructs_from_temporary_v<Types, UTypes&&> || ...) is true."
// Initializing a const int& element from a long would bind it to a temporary.
#include <tuple>

void f() {
  std::tuple<const int&> t(1L);
  (void)t;
}
