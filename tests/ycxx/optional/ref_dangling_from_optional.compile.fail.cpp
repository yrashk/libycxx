// EXPECT-ERROR-GCC: error: use of deleted function [^\n]*std::optional[^\n]*optional\(const std::optional
// EXPECT-ERROR-CLANG: error: call to deleted constructor of 'std::optional<const int &>'
// [optional.ref.ctor]/13: optional(const optional<U>&) "is defined as deleted if
// reference_constructs_from_temporary_v<T&, const U&> is true."
#include <optional>

void f() {
  const std::optional<long> src(1);
  std::optional<const int&> o(src);  // const int& from const long& binds to a temporary
  (void)o;
}
