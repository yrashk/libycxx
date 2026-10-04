// [format.string.std] Table 110: d is not a floating-point presentation type.
#include <format>
#include <string>

int main() { (void)std::format("{:d}", 1.5); }
