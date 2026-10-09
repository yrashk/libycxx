// EXPECT-ERROR: error: static assertion failed[^\n]*std::format: an argument type has no enabled formatter \(std::formattable is false\)
// [format.formatter.spec]/3 note and /5: formatter<wchar_t, char> is not provided (it
// would need an implicit wide-to-narrow conversion), so it is disabled.
#include <format>
#include <string>

int main() { (void)std::format("{}", L'x'); }
