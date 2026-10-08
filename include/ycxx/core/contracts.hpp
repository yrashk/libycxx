// libycxx core: <contracts> ([support.contract], P2900).
//
// contract_violation has the layout of the object GCC (-fcontracts) builds for a violated
// contract assertion and passes to ::handle_contract_violation(const contract_violation&)
// (learned from GCC 16's generated code): four 16-bit fields (a version, the assertion kind,
// the evaluation semantic, the detection mode, with the values of the enumerators below), the
// comment, a pointer to the source location's data (the object __builtin_source_location
// returns) and a pointer reserved for extensions. Its destructor is not virtual. Clang 23 has
// no contracts: there the header only declares the types and the default handler.
//
// The default handler ::handle_contract_violation is in the runtime archives, alone in its
// archive member, so a program's own definition replaces it. It calls
// invoke_default_contract_violation_handler, which reports the violation on stderr (hosted) or
// does nothing (freestanding runtime).
#pragma once

#include <ycxx/core/source_location.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace contracts {

enum class assertion_kind : unsigned short { pre = 1, post = 2, assert = 3 };
enum class evaluation_semantic : unsigned short { ignore = 1, observe = 2, enforce = 3, quick_enforce = 4 };
enum class detection_mode : unsigned short { predicate_false = 1, evaluation_exception = 2 };

class contract_violation {
  unsigned short __version_;
  unsigned short __kind_;
  unsigned short __semantic_;
  unsigned short __mode_;
  const char* __comment_;
  const void* __location_;
  void* __ext_;

public:
  contract_violation(const contract_violation&) = delete;
  contract_violation& operator=(const contract_violation&) = delete;
  ~contract_violation() = default;

  const char* comment() const noexcept { return __comment_ != nullptr ? __comment_ : ""; }
  contracts::detection_mode detection_mode() const noexcept { return static_cast<contracts::detection_mode>(__mode_); }
  bool is_terminating() const noexcept {
    return __semantic_ == static_cast<unsigned short>(evaluation_semantic::enforce) ||
           __semantic_ == static_cast<unsigned short>(evaluation_semantic::quick_enforce);
  }
  assertion_kind kind() const noexcept { return static_cast<assertion_kind>(__kind_); }
  source_location location() const noexcept { return source_location::__from_builtin(__location_); }
  evaluation_semantic semantic() const noexcept { return static_cast<evaluation_semantic>(__semantic_); }
};

void invoke_default_contract_violation_handler(const contract_violation& __v);

}}} // namespace std::contracts
