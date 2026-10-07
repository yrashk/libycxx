// [version.syn]/5: __cpp_lib_replaceable_contract_violation_handler is defined, to 202603L if
// the contract-violation handler is replaceable ([basic.contract.handler]/3) and to 0
// otherwise; [version.syn]/2: also by <contracts>. libycxx's handler is replaceable wherever
// the compiler has contract assertions (contracts/observe.pass.cpp replaces it).
#include <contracts>

#if !defined(__cpp_lib_replaceable_contract_violation_handler)
#  error "__cpp_lib_replaceable_contract_violation_handler is not defined by <contracts>"
#elif defined(__cpp_lib_contracts) && __cpp_lib_replaceable_contract_violation_handler != 202603L
#  error "__cpp_lib_replaceable_contract_violation_handler != 202603L"
#elif !defined(__cpp_lib_contracts) && __cpp_lib_replaceable_contract_violation_handler != 0
#  error "__cpp_lib_replaceable_contract_violation_handler != 0 without contracts"
#endif
