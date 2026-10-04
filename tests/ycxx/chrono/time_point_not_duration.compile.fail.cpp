// [time.point.general]/1: "If Duration is not a specialization of duration, the program is
// ill-formed."
#include <chrono>

std::chrono::time_point<std::chrono::system_clock, long long> tp{};

int main() {}
