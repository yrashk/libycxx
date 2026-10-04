// [unique.ptr.single.ctor]/12: "If D is a reference type, the second constructor is
// defined as deleted." Example 1: unique_ptr<int, const D&> p4(new int, D()); // error:
// rvalue deleter object combined with reference deleter type
#include <memory>

struct D {
  void operator()(int* p) const { delete p; }
};

int main() {
  std::unique_ptr<int, const D&> p4(new int, D());
}
