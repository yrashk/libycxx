// [obj.lifetime]/3: start_lifetime_as "Mandates: T is an implicit-lifetime type and not an
// incomplete type." A class whose only constructors are user-provided and that has a
// user-provided destructor is not an implicit-lifetime type ([class.prop]/8).
#include <memory>

struct NotImplicit {
  NotImplicit();
  ~NotImplicit();
  int x;
};

void f(void* p) { (void)std::start_lifetime_as<NotImplicit>(p); }
