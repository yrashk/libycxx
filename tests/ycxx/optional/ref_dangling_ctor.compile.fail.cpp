// EXPECT-ERROR-GCC: error: use of deleted function [^\n]*std::optional[^\n]*optional\(_Up&&\)
// EXPECT-ERROR-CLANG: error: call to deleted constructor of 'std::optional<const int &>'
// [optional.ref.ctor]/7: optional(U&&) "is defined as deleted if
// reference_constructs_from_temporary_v<T&, U> is true."
#include <optional>

void f() {
  std::optional<const int&> o(42L);  // would bind to a temporary int
  (void)o;
}
