// [format.string.std]/5: the sign option is invalid for charT unless an integer
// presentation type is specified.
#include <format>
#include <string>

int main() { (void)std::format("{:+}", 'c'); }
