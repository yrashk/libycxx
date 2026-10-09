// EXPECT-ERROR: error: static assertion failed[^\n]*duration: Period must be positive
// [time.duration.general]/3: "If Period::num is not positive, the program is ill-formed."
#include <chrono>
#include <ratio>

std::chrono::duration<int, std::ratio<-1, 2>> d{};

int main() {}
