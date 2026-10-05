// [util.smartptr.shared.mod]/3: reset(Y* p) is "Equivalent to shared_ptr(p).swap(*this)", and
// shared_ptr(Y*) requires "delete p" to be well-formed ([util.smartptr.shared.const]/3), which
// it is not for a void* ([expr.delete]/2). So shared_ptr<const void>::reset(void*) is
// ill-formed. (shared_ptr_void_pointer.compile.pass.cpp: the well-formed neighbours.)
// COUNTERPART: libstdcxx:20_util/shared_ptr/modifiers/reset_sfinae.cc
#include <memory>

void f(std::shared_ptr<const void>& p, void* v) {
  p.reset(v);
}

int main() {}
