// [format.tuple] Table 116: the m tuple-type requires sizeof...(Ts) == 2.
#include <format>
#include <string>
#include <tuple>

int main() { (void)std::format("{:m}", std::tuple<int, int, int>(1, 2, 3)); }
