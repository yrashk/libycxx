// libycxx freestanding runtime: the default contract-violation handler's report, which a
// freestanding program has nowhere to write: it does nothing (an enforced violation still
// terminates the program afterwards). The hosted runtime writes to stderr instead.
#include <contracts>

void std::contracts::invoke_default_contract_violation_handler(const contract_violation&) {}
