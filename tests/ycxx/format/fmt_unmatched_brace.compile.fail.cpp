// [format.string.general]/1: a lone } is neither an escape sequence nor part of a
// replacement field, so the string is not a format string.
#include <format>
#include <string>

int main() { (void)std::format("a } b", 1); }
