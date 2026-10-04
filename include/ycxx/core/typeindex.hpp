// libycxx core: <typeindex> ([type.index]). A thin wrapper over type_info; usable wherever a
// type_info can be obtained (that is, with RTTI).
#pragma once

#include <ycxx/core/compare.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/typeinfo.hpp>

namespace std {

class type_index {
  const type_info* target_;

public:
  type_index(const type_info& rhs) noexcept : target_(&rhs) {}

  bool operator==(const type_index& rhs) const noexcept { return *target_ == *rhs.target_; }
  bool operator<(const type_index& rhs) const noexcept { return target_->before(*rhs.target_); }
  bool operator>(const type_index& rhs) const noexcept { return rhs.target_->before(*target_); }
  bool operator<=(const type_index& rhs) const noexcept { return !rhs.target_->before(*target_); }
  bool operator>=(const type_index& rhs) const noexcept { return !target_->before(*rhs.target_); }
  strong_ordering operator<=>(const type_index& rhs) const noexcept {
    if (*target_ == *rhs.target_)
      return strong_ordering::equal;
    if (target_->before(*rhs.target_))
      return strong_ordering::less;
    return strong_ordering::greater;
  }

  size_t hash_code() const noexcept { return target_->hash_code(); }
  const char* name() const noexcept { return target_->name(); }
};

template <>
struct hash<type_index> {
  size_t operator()(const type_index& index) const noexcept { return index.hash_code(); }
};

} // namespace std
