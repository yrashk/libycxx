// libycxx core: ranges::dangling and borrowed_iterator_t ([range.dangling]), needed by the
// range algorithms in <memory> and <algorithm> before <ranges> is included.
#pragma once

#include <ycxx/core/range_access.hpp>

namespace std::ranges {

struct dangling {
  constexpr dangling() noexcept = default;
  constexpr dangling(auto&&...) noexcept {}
};

template <range R>
using borrowed_iterator_t = conditional_t<borrowed_range<R>, iterator_t<R>, dangling>;

} // namespace std::ranges
