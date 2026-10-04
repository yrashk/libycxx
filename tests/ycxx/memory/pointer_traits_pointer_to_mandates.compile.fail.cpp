// [pointer.traits.functions]/1: pointer_traits<Ptr>::pointer_to: "Mandates: For the first
// member function, Ptr::pointer_to(r) is well-formed." Here Ptr has no pointer_to.
#include <memory>

template <class T>
struct NoPointerTo {
  using element_type = T;
  T* raw;
};
int x;
auto p = std::pointer_traits<NoPointerTo<int>>::pointer_to(x);
