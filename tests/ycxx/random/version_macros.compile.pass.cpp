// [version.syn]: __cpp_lib_philox_engine 202406L and __cpp_lib_ranges_generate_random 202403L
// are defined in <random> (and <version>).
#include <random>

#if !defined(__cpp_lib_philox_engine) || __cpp_lib_philox_engine < 202406L
#error "__cpp_lib_philox_engine"
#endif
#if !defined(__cpp_lib_ranges_generate_random) || __cpp_lib_ranges_generate_random < 202403L
#error "__cpp_lib_ranges_generate_random"
#endif
#if !defined(__cpp_lib_freestanding_random) && __STDC_HOSTED__ == 0
#error "__cpp_lib_freestanding_random"
#endif

int main() {}
