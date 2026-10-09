// EXPECT-ERROR: error: static assertion failed[^\n]*std::default_delete: T must be a complete type
// [unique.ptr.dltr.dflt]/4: default_delete<T>::operator()(T*): "Mandates: T is a complete
// type." Destroying a unique_ptr<Incomplete> instantiates it.
#include <memory>

struct Incomplete;

void destroy(Incomplete* p) {
  std::unique_ptr<Incomplete> u(p);
}

int main() {}
