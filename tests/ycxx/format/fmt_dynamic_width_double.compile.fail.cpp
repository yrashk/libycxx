// [format.string.std]/10: a dynamic width "is valid only if the corresponding formatting
// argument is of standard signed or unsigned integer type."
#include <format>
#include <string>

int main() { (void)std::format("{:{}}", 1, 2.0); }
