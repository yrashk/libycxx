// [exec.simple.counting.ctor]/2: ~simple_counting_scope() invokes terminate if the state is not
// joined, unused or unused-and-closed. [exec.counting.scopes.general]/1.7: a scope with which an
// association was made is open, and stays open when the association ends (count back to 0):
// only a started join moves it towards joined (/1.8, /1.12). So destroying a scope that was used
// and never joined terminates, even with no association left. [exec.spawn]: spawn(sndr, token)
// associates sndr (which may complete only with set_value() or set_stopped()) with the scope;
// just() completes at once, ending the association.
// EXPECT-TERMINATE: terminate handler called at the destruction
#include <exception>
#include <execution>
#include <optional>
#include "check.hpp"

namespace ex = std::execution;

int main() {
  std::optional<ex::simple_counting_scope> scope;
  scope.emplace();
  bool ran = false;
  ex::spawn(ex::just() | ex::then([&]() noexcept { ran = true; }), scope->get_token());
  CHECK(ran); // the association has ended: count is 0, state open
  std::set_terminate([] {
    dprintf(2, "terminate handler called at the destruction\n");
    abort();
  });
  scope.reset();
  return 0; // not reached
}
