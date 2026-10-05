// [unique.ptr.create]/5: "template<class T, class... Args> unspecified make_unique(Args&&...)
// = delete; Constraints: T is an array of known bound."
// EXPECT-ERROR-GCC: use of deleted function .*make_unique.*\[with T = int \[4\]
// EXPECT-ERROR-CLANG: call to deleted function 'make_unique'
#include <memory>

int main() {
  auto p = std::make_unique<int[4]>();
}
