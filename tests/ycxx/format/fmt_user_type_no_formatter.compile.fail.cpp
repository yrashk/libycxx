// EXPECT-ERROR: error: static assertion failed[^\n]*std::format: an argument type has no enabled formatter \(std::formattable is false\)
// [format.formatter.spec]/5 and Example 1: "std::format("{}", err{}); // error: disabled
// formatter".
#include <format>
#include <string>

struct err {};

int main() { (void)std::format("{}", err{}); }
