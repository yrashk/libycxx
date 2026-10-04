// [utility.syn]: "template<class T> void as_const(const T&&) = delete;"
#include <utility>

void test() { (void)std::as_const(42); }
