// A program that asks for UBSan's vptr check builds and runs. The check (-fsanitize=vptr, part of
// GCC's -fsanitize=undefined) has the sanitizer runtime classify the object's type_info with
// dynamic_cast, through the __dynamic_cast of the C++ runtime that runtime was linked against
// (GCC's libubsan: libstdc++; Clang's libclang_rt.ubsan on Darwin: libc++abi), and that
// __dynamic_cast calls virtual functions of its own type_info classes through the object's
// type_info vtable. The Itanium C++ ABI (§2.9.4) fixes the type_info classes' data layout, not
// those virtual functions, and libycxx's vtables (src/abi/rtti_classes.cpp) have none of them:
// the call jumped into data (SIGBUS in __ubsan::checkDynamicType, observed with GCC 16 on
// Darwin) or freed a type_info object (Clang 23, -fsanitize=vptr). So tools/ycxx-cxx, the
// installed ycxx-c++ and ycxx::ycxx turn the check off (-fno-sanitize=vptr) whenever a sanitizer
// is requested (DECISIONS §6.8). This test asks for it explicitly and exercises what crashed:
// a virtual call through a base pointer, an iostream (basic_ios is polymorphic) and
// pmr::polymorphic_allocator (memory_resource's virtual allocate).
//   [expr.dynamic.cast], [class.virtual]: the calls below are well-formed and defined.
// FLAGS: -fsanitize=vptr -fno-sanitize-recover=all
#include <iostream>
#include <memory_resource>
#include <sstream>
#include <vector>
#include "check.hpp"

struct Base {
  virtual ~Base() = default;
  virtual int value() const { return 1; }
};
struct Derived : Base {
  int value() const override { return 2; }
};

int main() {
  Derived d;
  Base* b = &d;
  CHECK(b->value() == 2);
  CHECK(dynamic_cast<Derived*>(b) == &d);

  std::ostringstream out;
  out << "vptr " << 42;
  CHECK(out.str() == "vptr 42");
  std::cout << out.str() << '\n';

  std::pmr::monotonic_buffer_resource arena;
  std::pmr::vector<int> v(&arena);
  for (int i = 0; i < 100; ++i)
    v.push_back(i);
  CHECK(v.size() == 100 && v[99] == 99);
  return 0;
}
