// EXPECT-ERROR-GCC: error: use of deleted function [^\n]*std::tuple[^\n]*::tuple\(
// EXPECT-ERROR-CLANG: error: call to deleted constructor of [^\n]*std::tuple
// [tuple.cnstr]/27: the pair converting constructor "is defined as deleted if
// reference_constructs_from_temporary_v<T0, decltype(get<0>(FWD(u)))> || ... is true".
#include <tuple>
#include <utility>

void f() {
  std::pair<long, int> p(1, 2);
  std::tuple<const int&, int> t(p);
  (void)t;
}
