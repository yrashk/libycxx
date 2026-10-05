// [except.handle]/3.1: a handler of type cv T& matches when "E and T are the same type
// (ignoring the top-level cv-qualifiers)"; for a pointer exception object such a handler
// binds to the exception object itself ([except.handle]/15.2, Note 5: "any changes to the
// referenced object are changes to the exception object"), visible after `throw;`.
// REQUIRES: exceptions
// XFAIL: any  Itanium ABI limit: a handler type is recorded without its reference-ness ([except.handle]/3; STATUS; libsupc++ alike)
#include "check.hpp"

int main() {
  int i = 0;
  int which = 0;
  try {
    try {
      throw &i;
    } catch (const int*&) {
      which = -1;
    } catch (void*&) {
      which = -2;
    }
  } catch (int*& p) {  // exact type: binds to the exception object itself
    which = p == &i ? 1 : -3;
    p = nullptr;
    try {
      throw;
    } catch (int* q) {
      which = q == nullptr ? 1 : -4;  // modification visible through the rethrown object
    }
  }
  CHECK(which == 1);
  return 0;
}
