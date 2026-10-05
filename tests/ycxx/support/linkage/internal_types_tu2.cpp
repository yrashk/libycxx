// Second translation unit of except/handler_internal_linkage_types: classes with internal
// linkage whose names, layouts and bases are the same as classes of the first unit.
#include <typeinfo>
#include "internal_types_shared.hpp"

namespace {
struct Local {
  int v = 2;
  virtual ~Local() = default;
};
struct Hidden : Shared {
  int h = 22;
  Hidden() { s = 20; }
};
template <class T>
struct Box {
  T t;
};
}  // namespace

void tu2_throw_local() { throw Local(); }
void tu2_throw_hidden() { throw Hidden(); }
void tu2_throw_hidden_ptr() {
  static Hidden h;
  throw &h;
}
void tu2_throw_box() { throw Box<int>{3}; }
Shared* tu2_make_hidden() { return new Hidden; }
const std::type_info& tu2_local_type() { return typeid(Local); }
const std::type_info& tu2_box_type() { return typeid(Box<int>); }
bool tu2_catches_local(void (*f)()) {
  try {
    f();
  } catch (Local&) {
    return true;
  } catch (...) {
  }
  return false;
}
