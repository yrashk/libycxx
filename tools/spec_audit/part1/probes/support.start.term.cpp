// [support.start.term]: atexit/at_quick_exit take both a C and a C++ language linkage handler and
// are noexcept; exit, quick_exit, _Exit, abort are [[noreturn]] (freestanding, [cstdlib.syn]).
// [stdbit.h.syn]: the C bit utilities, global names, freestanding.
// FREESTANDING
#include <cstdlib>
#include <stdbit.h>
#include <type_traits>

extern "C" void c_handler();
void cxx_handler();
static_assert(noexcept(std::atexit(c_handler)) && noexcept(std::atexit(cxx_handler)));
static_assert(noexcept(std::at_quick_exit(c_handler)) && noexcept(std::at_quick_exit(cxx_handler)));
static_assert(std::is_same_v<decltype(std::atexit(cxx_handler)), int>);
static_assert(noexcept(std::quick_exit(0)) && noexcept(std::_Exit(0)) && noexcept(std::abort()));
static_assert(EXIT_SUCCESS == 0 && EXIT_FAILURE != 0);

// [stdbit.h.syn]
static_assert(__STDC_VERSION_STDBIT_H__ >= 202311L);
static_assert(__STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_LITTLE__ || __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_BIG__);
static_assert(std::is_same_v<decltype(stdc_leading_zeros_uc(0)), unsigned int>);
static_assert(std::is_same_v<decltype(stdc_bit_floor_ull(0)), unsigned long long>);
static_assert(std::is_same_v<decltype(stdc_has_single_bit_ui(1u)), bool>);
static_assert(std::is_unsigned_v<decltype(stdc_count_ones(0u))>);   // /1: an unsigned type (Mandates, not constraints: not probed)
static_assert(std::is_same_v<decltype(stdc_bit_ceil(static_cast<unsigned char>(0))), unsigned char>);
