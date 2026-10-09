// EXPECT-ERROR: error: static assertion failed[^\n]*std::start_lifetime: T must be an implicit\-lifetime aggregate type
// [obj.lifetime]/1: start_lifetime "Mandates: T is a complete type and an implicit-lifetime
// aggregate type." int is implicit-lifetime but not an aggregate.
#include <memory>

void f() {
  int i = 0;
  std::start_lifetime(i);
}
