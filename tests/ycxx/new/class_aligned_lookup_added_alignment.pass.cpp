// XFAIL-COMPILER: gcc  GCC 16.2 does not retry with an added align_val_t argument ([expr.new]/20.2)
// XFAIL-COMPILER: clang  Clang 23.1 does not retry with an added align_val_t argument ([expr.new]/20.2)
// [expr.new]/20: "If no matching function is found then (20.1) if the allocated object type has
// new-extended alignment, the alignment argument is removed from the argument list; (20.2)
// otherwise, an argument that is the type's alignment and has type std::align_val_t is added into
// the argument list immediately after the first argument; and then overload resolution is
// performed again." [expr.delete]/9.2: for a type without new-extended alignment a function
// without an align_val_t parameter is preferred, but if none is found the remaining (aligned)
// function is selected (9.3).
#include <new>
#include <cstddef>
#include <cstdlib>
#include "check.hpp"

static int calls[2];
static std::size_t last_align = 0;

// (D) default-aligned, only the aligned class functions: (20.2) adds the alignment argument.
struct OnlyAligned {
  int i;
  static void* operator new(std::size_t n, std::align_val_t a) {
    ++calls[0];
    last_align = static_cast<std::size_t>(a);
    return std::malloc(n);
  }
  static void operator delete(void* p, std::align_val_t) { ++calls[1]; std::free(p); }
};
int main() {
  last_align = 0;
  OnlyAligned* d = new OnlyAligned;
  CHECK(calls[0] == 1 && last_align == alignof(OnlyAligned));
  delete d;   // (9.2): no unaligned function found, so the aligned one remains
  CHECK(calls[1] == 1);
  return 0;
}
