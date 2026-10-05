// FLAGS: -fcontracts -fcontract-evaluation-semantic=observe
// XFAIL-COMPILER: clang  Clang 23 has no contract assertions (-fcontracts is unknown)
// [basic.contract.eval]/11: with the observe semantic a violation invokes the contract-violation
// handler with a contract_violation v; /14 Note 10: when it returns normally, execution continues.
// [basic.contract.eval]/12: a violation by an exception from the predicate invokes the handler
// within an active implicit handler for that exception (current_exception() refers to it).
// [support.contract.violation]: kind() is pre/post/assert after the syntactic form (Table 44),
// semantic() is observe (Table 45), is_terminating() is false for observe (a checking but not a
// terminating semantic, [basic.contract.eval]/1), detection_mode() is predicate_false or
// evaluation_exception (Table 46), comment() is an NTMBS.
// [basic.contract.handler]: the handler is ::handle_contract_violation; whether it is replaceable
// is implementation-defined (GCC: replaceable); this test replaces it.
// [support.contract.invoke]: invoke_default_contract_violation_handler invokes the default
// handler, which (Recommended practice, [basic.contract.handler]/2) returns normally.
// REQUIRES: exceptions
// XFAIL: gcc  GCC 16 reports a contract predicate that throws with detection_mode::predicate_false, not evaluation_exception ([support.contract.violation]; STATUS)
#include <contracts>
#include <exception>
#include "check.hpp"

namespace c = std::contracts;

int calls = 0;
c::assertion_kind last_kind{};
c::evaluation_semantic last_sem{};
c::detection_mode last_mode{};
bool last_terminating = true;
bool had_exception = false;
bool comment_ok = false;
bool call_default = false;

void handle_contract_violation(const c::contract_violation& v) {
  ++calls;
  last_kind = v.kind();
  last_sem = v.semantic();
  last_mode = v.detection_mode();
  last_terminating = v.is_terminating();
  had_exception = static_cast<bool>(std::current_exception());
  comment_ok = v.comment() != nullptr;
  (void)v.location();
  if (call_default) c::invoke_default_contract_violation_handler(v);
}

int f(int x) pre(x > 0) post(r : r > 10) { return x; }

bool throwing_predicate(int x) {
  if (x < 0) throw 1;
  return true;
}
int g(int x) pre(throwing_predicate(x)) { return x; }

int main() {
  CHECK(f(20) == 20 && calls == 0);

  CHECK(f(-1) == -1);  // both the precondition and the postcondition are violated
  CHECK(calls == 2);
  CHECK(last_kind == c::assertion_kind::post);
  CHECK(last_sem == c::evaluation_semantic::observe);
  CHECK(last_mode == c::detection_mode::predicate_false);
  CHECK(!last_terminating && !had_exception && comment_ok);

  calls = 0;
  CHECK(f(5) == 5);  // postcondition only
  CHECK(calls == 1 && last_kind == c::assertion_kind::post);

  calls = 0;
  CHECK(g(1) == 1 && calls == 0);
  CHECK(g(-1) == -1);
  CHECK(calls == 1 && last_kind == c::assertion_kind::pre);
  CHECK(last_mode == c::detection_mode::evaluation_exception);
  CHECK(had_exception);
  CHECK(!std::current_exception());  // the implicit handler is no longer active

  calls = 0;
  int y = 0;
  contract_assert(y == 1);
  CHECK(calls == 1 && last_kind == c::assertion_kind::assert);
  CHECK(last_mode == c::detection_mode::predicate_false && !last_terminating);

  // The default handler returns normally under observe, and execution continues.
  call_default = true;
  contract_assert(y == 2);
  CHECK(calls == 2);
  return 0;
}
