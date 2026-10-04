// [version.syn]: __cpp_lib_chrono is 202306L and __cpp_lib_chrono_udls 201304L, both also in
// <chrono>.
#include <chrono>

#if !defined(__cpp_lib_chrono) || __cpp_lib_chrono < 202306L
#error "__cpp_lib_chrono"
#endif
#if !defined(__cpp_lib_chrono_udls) || __cpp_lib_chrono_udls < 201304L
#error "__cpp_lib_chrono_udls"
#endif

int main() {}
