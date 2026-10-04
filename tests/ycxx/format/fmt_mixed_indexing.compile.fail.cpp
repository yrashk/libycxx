// [format.string.general]/4, Example 2: format("{0} to {}", "a", "b") is "not a format
// string (mixing automatic and manual indexing), ill-formed".
#include <format>
#include <string>

int main() { (void)std::format("{0} to {}", "a", "b"); }
