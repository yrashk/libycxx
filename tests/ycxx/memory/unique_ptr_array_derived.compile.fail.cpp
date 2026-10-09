// EXPECT-ERROR-GCC: error: no matching function.*std::unique_ptr<Base \[\]>::unique_ptr\(Derived\*\)
// EXPECT-ERROR-CLANG: error: no matching constructor for initialization of 'std::unique_ptr<Base\[\]>'
// [unique.ptr.runtime.general]/1.2: "Pointers to types derived from T are rejected by the
// constructors, and by reset." ([unique.ptr.runtime.ctor]/2: U must be pointer, or V* with
// V(*)[] convertible to element_type(*)[].)
#include <memory>

struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {};

int main() {
  std::unique_ptr<Base[]> p(new Derived[2]);
}
