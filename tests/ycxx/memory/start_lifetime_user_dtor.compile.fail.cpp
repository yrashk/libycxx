// EXPECT-ERROR: error: static assertion failed[^\n]*std::start_lifetime: T must be an implicit\-lifetime aggregate type
// [obj.lifetime]/1: start_lifetime "Mandates: T is a complete type and an implicit-lifetime
// aggregate type." An aggregate with a user-provided destructor is not implicit-lifetime
// ([class.prop]/8).
#include <memory>

struct A { int x; ~A() {} };
union U { char c; A a; ~U() {} };

void f() {
  U u{.c = 0};
  std::start_lifetime(u.a);
}
