// [basic.string.general]: "basic_string(nullptr_t) = delete;"
// EXPECT-ERROR-GCC: use of deleted function .*basic_string\(nullptr_t\)
// EXPECT-ERROR-CLANG: call to deleted constructor of 'std::string'
#include <string>

std::string s(nullptr);
