// [format.string.general]/2: "If there is no argument with the index arg-id in args, the
// string is not a format string for args." ([format.fmt.string]/3: ill-formed.)
#include <format>
#include <string>

int main() { (void)std::format("{} {}", 1); }
