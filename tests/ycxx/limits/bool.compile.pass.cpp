// [numeric.special]/3: "The specialization for bool shall be provided as follows:" (every
// member value is given verbatim in the draft).
#include <limits>
#include <type_traits>

using L = std::numeric_limits<bool>;
static_assert(L::is_specialized);
static_assert(L::min() == false && L::max() == true && L::lowest() == false);
static_assert(std::is_same_v<decltype(L::min()), bool>);
static_assert(L::digits == 1 && L::digits10 == 0 && L::max_digits10 == 0);
static_assert(!L::is_signed && L::is_integer && L::is_exact && L::radix == 2);
static_assert(L::epsilon() == 0 && L::round_error() == 0);
static_assert(L::min_exponent == 0 && L::min_exponent10 == 0 && L::max_exponent == 0 && L::max_exponent10 == 0);
static_assert(!L::has_infinity && !L::has_quiet_NaN && !L::has_signaling_NaN);
static_assert(L::infinity() == 0 && L::quiet_NaN() == 0 && L::signaling_NaN() == 0 && L::denorm_min() == 0);
static_assert(!L::is_iec559 && L::is_bounded && !L::is_modulo && !L::traps && !L::tinyness_before);
static_assert(L::round_style == std::round_toward_zero);
