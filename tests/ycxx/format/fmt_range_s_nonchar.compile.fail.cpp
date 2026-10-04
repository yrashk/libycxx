// [format.range.formatter] Table 115: the s range-type requires "T shall be charT".
#include <format>
#include <string>
#include <vector>

int main() { (void)std::format("{:s}", std::vector<int>{1, 2}); }
