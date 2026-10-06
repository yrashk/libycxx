// libycxx runtime: the default contract-violation handler ([basic.contract.handler]), in
// libycxx.a and the freestanding runtime archive. GCC (-fcontracts) calls
// ::handle_contract_violation for a violated contract assertion with the observe or enforce
// semantic. It is replaceable, so it is alone in its archive member: a program's definition is
// linked instead.
#include <contracts>

[[__gnu__::__visibility__("hidden")]] void handle_contract_violation(const std::contracts::contract_violation& __v);

[[__gnu__::__visibility__("hidden")]] void handle_contract_violation(const std::contracts::contract_violation& __v) {
  std::contracts::invoke_default_contract_violation_handler(__v);
}
