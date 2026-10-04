// [time.duration.literals]/3: "If any of these suffixes are applied to an integer-literal and the
// resulting chrono::duration value cannot be represented in the result type because of overflow,
// the program is ill-formed." (Assumes nanoseconds::rep is no wider than 64 bits, so 2^64 - 1 does
// not fit; [time.syn] requires at least 64 bits.)
#include <chrono>

using namespace std::chrono_literals;
auto x = 18446744073709551615ns;

int main() {}
