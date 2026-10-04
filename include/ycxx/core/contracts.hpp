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

namespace std::contracts {

enum class assertion_kind : unsigned short { pre = 1, post = 2, assert = 3 };
enum class evaluation_semantic : unsigned short { ignore = 1, observe = 2, enforce = 3, quick_enforce = 4 };
enum class detection_mode : unsigned short { predicate_false = 1, evaluation_exception = 2 };

class contract_violation {
  unsigned short version_;
  unsigned short kind_;
  unsigned short semantic_;
  unsigned short mode_;
  const char* comment_;
  const void* location_;
  void* ext_;

public:
  contract_violation(const contract_violation&) = delete;
  contract_violation& operator=(const contract_violation&) = delete;
  ~contract_violation() = default;

  const char* comment() const noexcept { return comment_ != nullptr ? comment_ : ""; }
  contracts::detection_mode detection_mode() const noexcept { return static_cast<contracts::detection_mode>(mode_); }
  bool is_terminating() const noexcept {
    return semantic_ == static_cast<unsigned short>(evaluation_semantic::enforce) ||
           semantic_ == static_cast<unsigned short>(evaluation_semantic::quick_enforce);
  }
  assertion_kind kind() const noexcept { return static_cast<assertion_kind>(kind_); }
  source_location location() const noexcept { return source_location::from_builtin(location_); }
  evaluation_semantic semantic() const noexcept { return static_cast<evaluation_semantic>(semantic_); }
};

void invoke_default_contract_violation_handler(const contract_violation& v);

} // namespace std::contracts
