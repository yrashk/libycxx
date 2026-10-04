// [format.string.general]/5: "If format-spec does not conform to the format specifications
// for the argument type referred to by arg-id, the string is not a format string for args";
// [format.fmt.string]/3: the consteval constructor is then not a constant expression.
// Table 107 has no s type for integers.
#include <format>
#include <string>

int main() { (void)std::format("{:s}", 42); }
