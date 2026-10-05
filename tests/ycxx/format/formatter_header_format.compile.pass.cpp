// [format.syn] declares the template formatter, so [format.formatter.spec]/2-/3 apply to
// <format> with nothing else included (formatter_spec.hpp).
#include <format>
#include "formatter_spec.hpp"

static_assert(formatter_spec::check());
