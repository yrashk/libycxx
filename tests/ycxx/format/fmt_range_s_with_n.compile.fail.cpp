// [format.range.formatter]/6: "If the range-type is s or ?s, then there shall be no n option
// and no range-underlying-spec."
#include <format>
#include <string>
#include <vector>

int main() { (void)std::format("{:ns}", std::vector<char>{'a'}); }
