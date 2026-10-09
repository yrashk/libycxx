// EXPECT-ERROR: error: static assertion failed[^\n]*std::format: an argument type has no enabled formatter \(std::formattable is false\)
// [format.formatter.spec]/4 and Example 1: formatter<const wchar_t*, char> is disabled, so
// format("{}", L"foo") is an error.
#include <format>
#include <string>

int main() { (void)std::format("{}", L"foo"); }
