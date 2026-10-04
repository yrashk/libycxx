// [time.duration.general]/3: "If Period is not a specialization of ratio, the program is ill-formed."
#include <chrono>

struct fake_ratio {
  static constexpr long long num = 1, den = 1;
  using type = fake_ratio;
};
std::chrono::duration<int, fake_ratio> d{};

int main() {}
