// [functional.syn]: "template<class T> void ref(const T&&) = delete;"
#include <functional>

auto r = std::ref(42);
