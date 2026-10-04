// [functional.syn]: "template<class T> void cref(const T&&) = delete;"
#include <functional>

auto r = std::cref(42);
