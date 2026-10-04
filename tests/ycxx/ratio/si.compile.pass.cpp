// [ratio.syn], [ratio.si]: the SI typedefs atto ... exa (all representable in a 64-bit
// intmax_t) are ratio specializations with the specified values.
#include <ratio>
#include <type_traits>

static_assert(std::is_same_v<std::atto, std::ratio<1, 1'000'000'000'000'000'000>>);
static_assert(std::is_same_v<std::femto, std::ratio<1, 1'000'000'000'000'000>>);
static_assert(std::is_same_v<std::pico, std::ratio<1, 1'000'000'000'000>>);
static_assert(std::is_same_v<std::nano, std::ratio<1, 1'000'000'000>>);
static_assert(std::is_same_v<std::micro, std::ratio<1, 1'000'000>>);
static_assert(std::is_same_v<std::milli, std::ratio<1, 1'000>>);
static_assert(std::is_same_v<std::centi, std::ratio<1, 100>>);
static_assert(std::is_same_v<std::deci, std::ratio<1, 10>>);
static_assert(std::is_same_v<std::deca, std::ratio<10, 1>>);
static_assert(std::is_same_v<std::hecto, std::ratio<100, 1>>);
static_assert(std::is_same_v<std::kilo, std::ratio<1'000, 1>>);
static_assert(std::is_same_v<std::mega, std::ratio<1'000'000, 1>>);
static_assert(std::is_same_v<std::giga, std::ratio<1'000'000'000, 1>>);
static_assert(std::is_same_v<std::tera, std::ratio<1'000'000'000'000, 1>>);
static_assert(std::is_same_v<std::peta, std::ratio<1'000'000'000'000'000, 1>>);
static_assert(std::is_same_v<std::exa, std::ratio<1'000'000'000'000'000'000, 1>>);
static_assert(std::ratio_multiply<std::kilo, std::milli>::num == 1 && std::ratio_multiply<std::kilo, std::milli>::den == 1);
