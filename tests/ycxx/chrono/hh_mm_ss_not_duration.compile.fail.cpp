// [time.hms.overview]/2: "If Duration is not a specialization of duration, the program is
// ill-formed."
#include <chrono>

std::chrono::hh_mm_ss<long long> h{};

int main() {}
