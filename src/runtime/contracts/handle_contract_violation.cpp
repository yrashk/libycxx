// libycxx runtime: the default contract-violation handler ([basic.contract.handler]), in
// libycxx.a and the freestanding runtime archive. GCC (-fcontracts) calls
// ::handle_contract_violation for a violated contract assertion with the observe or enforce
// semantic. It is replaceable, so it is alone in its archive member: a program's definition is
// linked instead.
#include <contracts>

[[gnu::visibility("hidden")]] void handle_contract_violation(const std::contracts::contract_violation& v);

[[gnu::visibility("hidden")]] void handle_contract_violation(const std::contracts::contract_violation& v) {
  std::contracts::invoke_default_contract_violation_handler(v);
}
