// [format.string.std]/7: the # option "is valid for arithmetic types other than charT and
// bool or when an integer presentation type is specified, and not otherwise."
#include <format>
#include <string>

int main() { (void)std::format("{:#}", true); }
