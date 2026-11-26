// Included within the namespace that owns the no-state tag ([thread.stoptoken]).
#pragma once

struct nostopstate_t {
  explicit nostopstate_t() = default;
};
inline constexpr nostopstate_t nostopstate{};
