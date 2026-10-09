// EXPECT-ERROR-GCC: error: use of deleted function [^\n]*std::unique_ptr[^\n]*unique_ptr\(const std::unique_ptr
// EXPECT-ERROR-CLANG: error: call to deleted constructor of 'std::unique_ptr<int>'
// [unique.ptr.single.general]: "unique_ptr(const unique_ptr&) = delete;" — a unique_ptr
// cannot be copied from an lvalue.
#include <memory>

int main() {
  std::unique_ptr<int> a(new int(1));
  std::unique_ptr<int> b(a);
}
