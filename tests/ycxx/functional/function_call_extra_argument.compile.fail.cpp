// EXPECT-ERROR-GCC: error: no match for call[^\n]*std::function<
// EXPECT-ERROR-CLANG: error: no matching function for call to object[^\n]*std::function<
// [func.wrap.func.general]: "R operator()(ArgTypes...) const;" -- function<int(int)> takes
// exactly one argument; calling it with two is ill-formed.
#include <functional>
#include <utility>

int f(const std::function<int(int)>& fn) { return fn(1, 2); }
