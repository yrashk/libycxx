// EXPECT-ERROR-GCC: error: no matching function.*std::unique_ptr<Base \[\]>::reset\(Derived\*\)
// EXPECT-ERROR-CLANG: error: no matching member function for call to 'reset'
// [unique.ptr.runtime.modifiers]/3: reset(U p) is constrained on "U is the same type as
// pointer, or pointer is the same type as element_type*, U is a pointer type V*, and
// V(*)[] is convertible to element_type(*)[]". Derived* -> Base(*)[] is not.
#include <memory>

struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {};

int main() {
  std::unique_ptr<Base[]> p;
  p.reset(new Derived[2]);
}
