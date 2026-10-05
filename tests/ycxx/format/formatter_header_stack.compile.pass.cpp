// [stack.syn] declares `template<class charT, class T, formattable<charT> Container> struct
// formatter<stack<T, Container>, charT>;` and `enable_nonlocking_formatter_optimization<stack<T,
// Container>> = false`, so <stack> declares the template formatter: with only <stack> included,
// [format.formatter.spec]/2-/3 apply (formatter_spec.hpp), and the adaptor's
// enable_nonlocking_formatter_optimization is false.
#include <stack>
#include "formatter_spec.hpp"

static_assert(!std::enable_nonlocking_formatter_optimization<std::stack<int>>);
static_assert(!std::enable_nonlocking_formatter_optimization<std::stack<char, std::stack<int>::container_type>>);
static_assert(formatter_spec::check());
