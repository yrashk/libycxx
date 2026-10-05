// [queue.syn] declares the formatter specializations of queue and priority_queue
// ([container.adaptors.format]) and `enable_nonlocking_formatter_optimization<queue<T, Container>>
// = false` (likewise for priority_queue), so with only <queue> included [format.formatter.spec]/2-/3
// apply (formatter_spec.hpp) and those two are false.
#include <queue>
#include "formatter_spec.hpp"

static_assert(!std::enable_nonlocking_formatter_optimization<std::queue<int>>);
static_assert(!std::enable_nonlocking_formatter_optimization<std::priority_queue<int>>);
static_assert(!std::enable_nonlocking_formatter_optimization<std::priority_queue<long, std::vector<long>, std::less<long>>>);
static_assert(formatter_spec::check());
