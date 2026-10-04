// [cwchar.syn]: "#define __STDC_VERSION_WCHAR_H__ 202311L"; NULL, WCHAR_MAX, WCHAR_MIN, WEOF
// and WCHAR_WIDTH (freestanding). C (7.31.1, 7.22.3): WCHAR_MIN/WCHAR_MAX are the limits of
// wchar_t, WCHAR_WIDTH its width; WEOF is "a constant expression of type wint_t whose value
// does not correspond to any member of the extended character set".
#include <cwchar>
#include <limits>
#include <type_traits>

#if !defined(__STDC_VERSION_WCHAR_H__) || __STDC_VERSION_WCHAR_H__ != 202311L
#  error "__STDC_VERSION_WCHAR_H__ must be 202311L"
#endif
#ifndef NULL
#  error "NULL"
#endif
#if !defined(WCHAR_MAX) || !defined(WCHAR_MIN) || !defined(WEOF)
#  error "WCHAR_MAX, WCHAR_MIN, WEOF"
#endif
#ifndef WCHAR_WIDTH
#  error "WCHAR_WIDTH"
#endif

static_assert(WCHAR_MAX == std::numeric_limits<wchar_t>::max());
static_assert(WCHAR_MIN == std::numeric_limits<wchar_t>::min());
static_assert(WCHAR_WIDTH == std::numeric_limits<wchar_t>::digits + std::numeric_limits<wchar_t>::is_signed);
static_assert(std::is_same_v<std::remove_cv_t<decltype(WEOF)>, std::wint_t>);
constexpr std::wint_t weof = WEOF;
static_assert(weof == WEOF);
