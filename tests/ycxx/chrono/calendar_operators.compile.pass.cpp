// [time.cal.operators]: the conventional-syntax operator/ overloads and their result types; for
// example "auto a = 2015/4/4; // a == int(125)", "auto b = 2015y/4/4; // year_month_day",
// "auto c = 2015y/4d/April; // error", "auto d = 2015/April/4; // error".
#include <chrono>
#include <type_traits>

using namespace std::chrono;
using std::is_same_v;

static_assert(is_same_v<decltype(2015y / April), year_month>);
static_assert(is_same_v<decltype(2015y / 4), year_month>);
static_assert(is_same_v<decltype(April / 4d), month_day>);
static_assert(is_same_v<decltype(April / 4), month_day>);
static_assert(is_same_v<decltype(4 / 4d), month_day>);
static_assert(is_same_v<decltype(4d / April), month_day>);
static_assert(is_same_v<decltype(4d / 4), month_day>);
static_assert(is_same_v<decltype(April / last), month_day_last>);
static_assert(is_same_v<decltype(4 / last), month_day_last>);
static_assert(is_same_v<decltype(last / April), month_day_last>);
static_assert(is_same_v<decltype(last / 4), month_day_last>);
static_assert(is_same_v<decltype(April / Monday[1]), month_weekday>);
static_assert(is_same_v<decltype(4 / Monday[1]), month_weekday>);
static_assert(is_same_v<decltype(Monday[1] / April), month_weekday>);
static_assert(is_same_v<decltype(Monday[1] / 4), month_weekday>);
static_assert(is_same_v<decltype(April / Monday[last]), month_weekday_last>);
static_assert(is_same_v<decltype(4 / Monday[last]), month_weekday_last>);
static_assert(is_same_v<decltype(Monday[last] / April), month_weekday_last>);
static_assert(is_same_v<decltype(Monday[last] / 4), month_weekday_last>);
static_assert(is_same_v<decltype(2015y / April / 4d), year_month_day>);
static_assert(is_same_v<decltype(2015y / April / 4), year_month_day>);
static_assert(is_same_v<decltype(2015y / (April / 4d)), year_month_day>);
static_assert(is_same_v<decltype(2015 / (April / 4d)), year_month_day>);
static_assert(is_same_v<decltype(April / 4d / 2015y), year_month_day>);
static_assert(is_same_v<decltype(April / 4d / 2015), year_month_day>);
static_assert(is_same_v<decltype(4d / April / 2015y), year_month_day>);
static_assert(is_same_v<decltype(2015y / April / last), year_month_day_last>);
static_assert(is_same_v<decltype(2015y / (April / last)), year_month_day_last>);
static_assert(is_same_v<decltype(2015 / (April / last)), year_month_day_last>);
static_assert(is_same_v<decltype(April / last / 2015y), year_month_day_last>);
static_assert(is_same_v<decltype(April / last / 2015), year_month_day_last>);
static_assert(is_same_v<decltype(2015y / April / Monday[1]), year_month_weekday>);
static_assert(is_same_v<decltype(2015y / (April / Monday[1])), year_month_weekday>);
static_assert(is_same_v<decltype(2015 / (April / Monday[1])), year_month_weekday>);
static_assert(is_same_v<decltype(April / Monday[1] / 2015y), year_month_weekday>);
static_assert(is_same_v<decltype(April / Monday[1] / 2015), year_month_weekday>);
static_assert(is_same_v<decltype(2015y / April / Monday[last]), year_month_weekday_last>);
static_assert(is_same_v<decltype(2015y / (April / Monday[last])), year_month_weekday_last>);
static_assert(is_same_v<decltype(2015 / (April / Monday[last])), year_month_weekday_last>);
static_assert(is_same_v<decltype(April / Monday[last] / 2015y), year_month_weekday_last>);
static_assert(is_same_v<decltype(April / Monday[last] / 2015), year_month_weekday_last>);
static_assert(is_same_v<decltype(2015 / 4 / 4), int>);
static_assert(2015 / 4 / 4 == 125);
static_assert(2015y / 4 / 4 == year_month_day{year(2015), month(4), day(4)});

template <class A, class B> concept divisible = requires(A a, B b) { a / b; };
static_assert(!divisible<year, day>);                 // 2015y/4d/April: no viable first /
static_assert(!divisible<int, month>);                // 2015/April/4: no viable first /
static_assert(!divisible<year, year> && !divisible<month, month> && !divisible<day, day>);
static_assert(!divisible<year_month_day, int>);
static_assert(!divisible<weekday, month>);  // a weekday needs an index

int main() {}
