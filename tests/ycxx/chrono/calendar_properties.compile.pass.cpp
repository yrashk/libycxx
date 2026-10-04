// [time.cal]: each calendar type "is a trivially copyable and standard-layout class type"; the
// default constructors of day, month, year, weekday, weekday_indexed, month_day, year_month,
// year_month_day, year_month_weekday are defaulted; last_spec has an explicit defaulted default
// constructor ([time.cal.last]); the constants of [time.syn] (last, Sunday..Saturday,
// January..December) are constexpr.
#include <chrono>
#include <type_traits>

using namespace std::chrono;

template <class T>
constexpr bool props = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

static_assert(props<day> && props<month> && props<year> && props<weekday> && props<weekday_indexed>);
static_assert(props<weekday_last> && props<month_day> && props<month_day_last> && props<month_weekday>);
static_assert(props<month_weekday_last> && props<year_month> && props<year_month_day>);
static_assert(props<year_month_day_last> && props<year_month_weekday> && props<year_month_weekday_last>);
static_assert(std::is_trivially_default_constructible_v<day> && std::is_trivially_default_constructible_v<month>);
static_assert(std::is_trivially_default_constructible_v<year> && std::is_trivially_default_constructible_v<weekday>);
static_assert(std::is_trivially_default_constructible_v<year_month_day>);
static_assert(std::is_trivially_default_constructible_v<year_month_weekday>);
static_assert(!std::is_default_constructible_v<weekday_last> && !std::is_default_constructible_v<month_day_last>);
static_assert(!std::is_default_constructible_v<year_month_day_last>);

static_assert(std::is_same_v<decltype(last), const last_spec>);
static_assert(std::is_default_constructible_v<last_spec>);
template <class T> concept implicit_default = requires(void (*f)(T)) { f({}); };
static_assert(!implicit_default<last_spec>);  // explicit last_spec() = default

constexpr weekday wds[] = {Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday};
constexpr month ms[] = {January, February, March, April, May, June, July, August, September, October, November, December};
consteval bool constants() {
  for (unsigned i = 0; i < 7; ++i)
    if (wds[i].c_encoding() != i) return false;
  for (unsigned i = 0; i < 12; ++i)
    if (unsigned(ms[i]) != i + 1) return false;
  return true;
}
static_assert(constants());

int main() {}
