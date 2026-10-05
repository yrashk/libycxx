// Classes with internal linkage (unnamed namespace, [basic.link]/4) in two translation units
// that have the same names, members and bases are different types ([basic.link]/11 applies
// only to entities with linkage that denote the same entity; [basic.def.odr]). So a handler
// for one unit's type does not match an exception object of the other's ([except.handle]/3),
// dynamic_cast to one unit's class fails for an object of the other's
// ([expr.dynamic.cast]/9), and their type_info objects compare unequal ([expr.typeid],
// [type.info]/2). A handler for their common base Shared (external linkage) matches, and the
// same holds for a class template specialization with an internal-linkage template.
// Results are checked repeatedly, in alternation, so remembered match results must not be
// keyed by the type's name.
// FILES: ../support/linkage/internal_types_tu2.cpp
// XFAIL-COMPILER: clang  type_info names of internal-linkage classes lack the '*' prefix that asks for address comparison
// REQUIRES: exceptions
#include <typeinfo>
#include "linkage/internal_types_shared.hpp"
#include "check.hpp"

namespace {
struct Local {
  int v = 1;
  virtual ~Local() = default;
};
struct Hidden : Shared {
  int h = 11;
  Hidden() { s = 10; }
};
template <class T>
struct Box {
  T t;
};
}  // namespace

static void throw_local() { throw Local(); }
static void throw_hidden() { throw Hidden(); }

// 1: own Local, 2: own Hidden, 3: own Box<int>, 4: Shared (value of s), 0: other
static int classify(void (*f)(), int* s = nullptr) {
  try {
    f();
  } catch (Local& l) {
    return l.v == 1 ? 1 : -1;
  } catch (Hidden* h) {
    return h->h == 11 ? 2 : -2;
  } catch (Hidden& h) {
    return h.h == 11 ? 2 : -2;
  } catch (Box<int>& b) {
    return b.t == 3 ? 3 : -3;
  } catch (Shared* p) {
    if (s) *s = p->s;
    return 5;
  } catch (Shared& sh) {
    if (s) *s = sh.s;
    return 4;
  } catch (...) {
    return 0;
  }
}

static void throw_box() { throw Box<int>{3}; }

int main() {
  for (int round = 0; round < 200; ++round) {
    int s = 0;
    CHECK(classify(throw_local) == 1);
    CHECK(classify(tu2_throw_local) == 0);
    CHECK(classify(throw_hidden, &s) == 2);
    CHECK(classify(tu2_throw_hidden, &s) == 4 && s == 20);
    s = 0;
    CHECK(classify(tu2_throw_hidden_ptr, &s) == 5 && s == 20);
    CHECK(classify(throw_box) == 3);
    CHECK(classify(tu2_throw_box) == 0);
    CHECK(tu2_catches_local(tu2_throw_local));
    CHECK(!tu2_catches_local(throw_local));
  }
  CHECK(typeid(Local) != tu2_local_type());
  CHECK(!(typeid(Local) == tu2_local_type()));
  CHECK(typeid(Box<int>) != tu2_box_type());
  Shared* other = tu2_make_hidden();
  Shared* own = new Hidden;
  for (int round = 0; round < 100; ++round) {
    CHECK(dynamic_cast<Hidden*>(other) == nullptr);
    CHECK(dynamic_cast<Hidden*>(own) != nullptr && dynamic_cast<Hidden*>(own)->h == 11);
    CHECK(typeid(*other) != typeid(Hidden) && typeid(*own) == typeid(Hidden));
    CHECK(dynamic_cast<void*>(other) == static_cast<void*>(other) || dynamic_cast<void*>(other) != nullptr);
  }
  delete other;
  delete own;
  return 0;
}
