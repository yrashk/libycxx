// [basic.string.general]: "basic_string& operator=(nullptr_t) = delete;"
// EXPECT-ERROR-GCC: use of deleted function .*operator=\(nullptr_t\)
// EXPECT-ERROR-CLANG: overload resolution selected deleted operator '='
#include <string>

void f(std::string& s) { s = nullptr; }
