// [ratio.ratio]/2: num is sgn(N) * sgn(D) * abs(N) / gcd and den is abs(D) / gcd; type is
// ratio<num, den>; D defaults to 1. num and den are static constexpr intmax_t.
#include <ratio>
#include <cstdint>
#include <type_traits>

using std::intmax_t;
static_assert(std::ratio<6, 4>::num == 3 && std::ratio<6, 4>::den == 2);
static_assert(std::ratio<-6, 4>::num == -3 && std::ratio<-6, 4>::den == 2);
static_assert(std::ratio<6, -4>::num == -3 && std::ratio<6, -4>::den == 2);
static_assert(std::ratio<-6, -4>::num == 3 && std::ratio<-6, -4>::den == 2);
static_assert(std::ratio<0, -7>::num == 0 && std::ratio<0, -7>::den == 1);
static_assert(std::ratio<5>::num == 5 && std::ratio<5>::den == 1);
static_assert(std::ratio<INTMAX_MAX, INTMAX_MAX>::num == 1 && std::ratio<INTMAX_MAX, INTMAX_MAX>::den == 1);
static_assert(std::ratio<-INTMAX_MAX, 1>::num == -INTMAX_MAX);
static_assert(std::ratio<1, -INTMAX_MAX>::num == -1 && std::ratio<1, -INTMAX_MAX>::den == INTMAX_MAX);
static_assert(std::is_same_v<std::ratio<6, 4>::type, std::ratio<3, 2>>);
static_assert(std::is_same_v<std::ratio<2, -4>::type, std::ratio<-1, 2>>);
static_assert(std::is_same_v<std::ratio<3, 2>::type, std::ratio<3, 2>>);
static_assert(!std::is_same_v<std::ratio<6, 4>, std::ratio<3, 2>>);  // distinct types, same value
static_assert(std::is_same_v<decltype(std::ratio<1, 2>::num), const intmax_t>);
static_assert(std::is_same_v<decltype(std::ratio<1, 2>::den), const intmax_t>);

constexpr const intmax_t* addr = &std::ratio<1, 3>::den;  // static data members are objects
static_assert(*addr == 3);
