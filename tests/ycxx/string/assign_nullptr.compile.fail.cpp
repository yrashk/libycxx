// [basic.string.general]: "basic_string& operator=(nullptr_t) = delete;"
#include <string>

void f(std::string& s) { s = nullptr; }
