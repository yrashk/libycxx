// [format.formatter.spec]/3 note and /5: formatter<wchar_t, char> is not provided (it
// would need an implicit wide-to-narrow conversion), so it is disabled.
#include <format>
#include <string>

int main() { (void)std::format("{}", L'x'); }
