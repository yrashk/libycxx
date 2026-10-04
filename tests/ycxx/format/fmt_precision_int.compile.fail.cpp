// [format.string.std]/15: "The precision option is valid for floating-point and string
// types." An integer argument with a precision is not a format string.
#include <format>
#include <string>

int main() { (void)std::format("{:.2}", 1); }
