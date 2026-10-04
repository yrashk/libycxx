// [format.string.std]/5: "The sign option is only valid for arithmetic types other than
// charT and bool or when an integer presentation type is specified."
#include <format>
#include <string>

int main() { (void)std::format("{:+}", "text"); }
