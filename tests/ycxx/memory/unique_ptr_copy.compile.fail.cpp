// [unique.ptr.single.general]: "unique_ptr(const unique_ptr&) = delete;" — a unique_ptr
// cannot be copied from an lvalue.
#include <memory>

int main() {
  std::unique_ptr<int> a(new int(1));
  std::unique_ptr<int> b(a);
}
