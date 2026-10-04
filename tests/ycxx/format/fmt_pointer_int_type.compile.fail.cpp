// [format.string.std] Table 111: pointers accept only none, p and P.
#include <format>
#include <string>

int main() { (void)std::format("{:x}", nullptr); }
