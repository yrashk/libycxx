// [ios.base.storage]: xalloc() returns a unique index ("index ++"); iword(idx) / pword(idx)
// return references to elements initialized to 0 / a null pointer, which stay associated with
// the object; [ios.base.callback]: register_callback; the destructor calls the callbacks with
// erase_event ([ios.base.cons]/2); imbue calls them with imbue_event ([ios.base.locales]).
#include <ios>
#include <locale>
#include <sstream>
#include "check.hpp"

static int erased = 0, imbued = 0;
static void cb(std::ios_base::event ev, std::ios_base& s, int idx) {
  if (ev == std::ios_base::erase_event) ++erased;
  if (ev == std::ios_base::imbue_event) {
    ++imbued;
    s.iword(idx) = 99;
  }
}

int main() {
  int i1 = std::ios_base::xalloc();
  int i2 = std::ios_base::xalloc();
  CHECK(i1 != i2);
  {
    std::ostringstream s;
    CHECK(s.iword(i1) == 0);
    CHECK(s.pword(i2) == nullptr);
    s.iword(i1) = 5;
    int x = 0;
    s.pword(i2) = &x;
    CHECK(s.iword(i1) == 5 && s.pword(i2) == &x);
    CHECK(s.iword(i1 + i2 + 100) == 0);  // a new element, initialized to zero
    CHECK(s.iword(i1) == 5);  // still associated
    s.register_callback(cb, i1);
    s.imbue(std::locale::classic());
    CHECK(imbued == 1 && s.iword(i1) == 99);
  }
  CHECK(erased == 1);
  return 0;
}
