// libycxx core: <typeindex> ([type.index]). A thin wrapper over type_info; usable wherever a
// type_info can be obtained (that is, with RTTI).
#pragma once

#include <ycxx/core/compare.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/typeinfo.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

class type_index {
  const type_info* __target_;

public:
  type_index(const type_info& __rhs) noexcept : __target_(&__rhs) {}

  bool operator==(const type_index& __rhs) const noexcept { return *__target_ == *__rhs.__target_; }
  bool operator<(const type_index& __rhs) const noexcept { return __target_->before(*__rhs.__target_); }
  bool operator>(const type_index& __rhs) const noexcept { return __rhs.__target_->before(*__target_); }
  bool operator<=(const type_index& __rhs) const noexcept { return !__rhs.__target_->before(*__target_); }
  bool operator>=(const type_index& __rhs) const noexcept { return !__target_->before(*__rhs.__target_); }
  strong_ordering operator<=>(const type_index& __rhs) const noexcept {
    if (*__target_ == *__rhs.__target_)
      return strong_ordering::equal;
    if (__target_->before(*__rhs.__target_))
      return strong_ordering::less;
    return strong_ordering::greater;
  }

  size_t hash_code() const noexcept { return __target_->hash_code(); }
  const char* name() const noexcept { return __target_->name(); }
};

template <>
struct hash<type_index> {
  size_t operator()(const type_index& index) const noexcept { return index.hash_code(); }
};

} // namespace std
