// FLAGS: -fcontracts -fcontract-evaluation-semantic=enforce
// XFAIL-COMPILER: clang  Clang 23 has no contract assertions (-fcontracts is unknown)
// [basic.contract.eval]/11: with the enforce semantic the handler is invoked; semantic() is
// enforce and is_terminating() is true ([support.contract.violation]/5: enforce is a terminating
// semantic). /17: a handler that exits via an exception from a function contract assertion
// behaves as if the function body exited via that exception; Note 13: from an assertion-statement
// the exception propagates from that statement. Throwing avoids contract termination (Note 11).
// REQUIRES: exceptions
#include <contracts>
#include "check.hpp"

namespace c = std::contracts;

struct Violation {
  c::assertion_kind kind;
  c::evaluation_semantic sem;
  bool terminating;
};

void handle_contract_violation(const c::contract_violation& v) {
  throw Violation{v.kind(), v.semantic(), v.is_terminating()};
}

int body_ran = 0;
int f(int x) pre(x > 0) {
  ++body_ran;
  return x;
}
int h(int x) post(r : r == 0) { return x; }

int main() {
  CHECK(f(1) == 1 && body_ran == 1);
  bool caught = false;
  try {
    f(-1);
  } catch (const Violation& v) {
    caught = v.kind == c::assertion_kind::pre && v.sem == c::evaluation_semantic::enforce && v.terminating;
  }
  CHECK(caught && body_ran == 1);

  caught = false;
  try {
    h(3);
  } catch (const Violation& v) {
    caught = v.kind == c::assertion_kind::post && v.terminating;
  }
  CHECK(caught);

  caught = false;
  int after = 0;
  try {
    contract_assert(after == 1);
    after = 1;
  } catch (const Violation& v) {
    caught = v.kind == c::assertion_kind::assert && v.sem == c::evaluation_semantic::enforce;
  }
  CHECK(caught && after == 0);
  return 0;
}
