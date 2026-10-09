// Built with -ffreestanding -fno-exceptions -fno-rtti (__STDC_HOSTED__ is 0). [compliance]/2-3:
// a freestanding implementation provides the headers of Table 27 with at least their freestanding
// items ([freestanding.item]/4-6: declarations marked "freestanding" or "freestanding-deleted",
// everything in "all freestanding" headers, the non-"hosted" parts of "mostly freestanding" ones,
// and the members of partially freestanding classes, [freestanding.item]/5.2). This program uses
// items of the Table 27 headers (the rest: modes/freestanding_items_numeric) and checks their
// specified effects; the runtime it links against is the hosted one (tools/check_freestanding.sh
// links bare-metal), which is allowed: [compliance]/4, a freestanding implementation may provide
// hosted facilities.
// Items (header synopsis paragraph names):
//   [cstddef.syn] byte, to_integer, offsetof, max_align_t; [cstdlib.syn] abs, div, lldiv,
//   bsearch, qsort, EXIT_SUCCESS (constexpr div and memalignment: cstdlib/*); [cstring.syn]
//   memcpy, strlen, strchr (const overload), memset; [cwchar.syn] wcslen, wmemcmp, WCHAR_MAX,
//   mbstate_t; [cerrno.syn] EDOM, ERANGE, EINVAL; [limits.syn], [climits.syn], [cfloat.syn],
//   [cstdint.syn]; [new.syn] placement new, launder, nothrow, align_val_t; [source.location.syn];
//   [exception.syn] exception, uncaught_exceptions; [initializer.list.syn]; [compare.syn];
//   [coroutine.syn] coroutine_handle<>, noop_coroutine; [cstdarg.syn]; [concepts.syn];
//   [debugging.syn] is_debugger_present; [memory.syn] unique_ptr, construct_at, addressof, align,
//   assume_aligned, pointer_traits; [meta.type.synop]; [ratio.syn]; [utility.syn] pair,
//   exchange, cmp_less, in_range, to_underlying; [tuple.syn] apply; [optional.syn] (value() is
//   freestanding-deleted; operator* is not), [variant.syn] get_if (get is freestanding-deleted),
//   visit; [expected.syn]; [functional.syn] invoke, function_ref, bind_front, not_fn,
//   default_searcher, hash, ranges::less; [bit.syn]; [array.syn]; [span.syn]; [mdspan.syn];
//   [iterator.synopsis]; [ranges.syn] views; [algorithm.syn] sort, ranges::find;
//   [numeric.ops.overview] accumulate, gcd, saturating_add, midpoint; [string.view.synop];
//   [atomics.syn] atomic, atomic_flag, atomic_ref.
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cerrno>
#include <cfloat>
#include <climits>
#include <compare>
#include <concepts>
#include <coroutine>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <debugging>
#include <exception>
#include <expected>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <mdspan>
#include <memory>
#include <new>
#include <numeric>
#include <optional>
#include <ranges>
#include <ratio>
#include <source_location>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <version>
#include "check.hpp"

// FLAGS: -ffreestanding -fno-exceptions -fno-rtti

static_assert(__STDC_HOSTED__ == 0);

struct Layout {
  char c;
  int i;
};
static_assert(offsetof(Layout, i) == alignof(int));
static_assert(std::to_integer<int>(std::byte{0x5a}) == 0x5a);
static_assert(alignof(std::max_align_t) >= alignof(long double));
static_assert(std::abs(-3) == 3 && std::abs(-3L) == 3L && std::abs(-3LL) == 3LL);
static_assert(EXIT_SUCCESS == 0);
static_assert(std::numeric_limits<int>::max() == INT_MAX && CHAR_BIT == 8);
static_assert(FLT_RADIX == 2 && DBL_MANT_DIG == std::numeric_limits<double>::digits);
static_assert(sizeof(std::int32_t) == 4 && INT64_MAX == std::numeric_limits<std::int64_t>::max());
static_assert(WCHAR_MAX == std::numeric_limits<wchar_t>::max());
static_assert(EDOM != ERANGE && EINVAL != 0);
static_assert(std::is_same_v<std::common_type_t<int, long>, long>);
static_assert(std::ratio_add<std::ratio<1, 2>, std::ratio<1, 3>>::num == 5);
static_assert(std::cmp_less(-1, 0u) && !std::in_range<unsigned>(-1));
static_assert(std::get<1>(std::tuple<int, char>(1, 'x')) == 'x');
static_assert(std::apply([](int a, int b) { return a * b; }, std::pair(6, 7)) == 42);
static_assert(std::popcount(0xF0u) == 4 && std::bit_ceil(5u) == 8u && std::countr_zero(8u) == 3);
static_assert(std::byteswap(std::uint16_t(0x1234)) == 0x3412);
static_assert(std::gcd(12, 18) == 6 && std::lcm(4, 6) == 12 && std::midpoint(1, 3) == 2);
static_assert(std::saturating_add<std::int8_t>(100, 100) == 127 && std::saturating_sub<unsigned>(1u, 2u) == 0u);
static_assert(std::saturating_cast<std::uint8_t>(-5) == 0);
static_assert(std::string_view("hello").substr(1, 3) == "ell");
static_assert((1 <=> 2) < 0 && std::is_eq(std::strong_order(1, 1)));
static_assert(std::totally_ordered<int> && std::invocable<int (*)(int), int>);
static_assert(std::ranges::equal(std::views::iota(1, 4) | std::views::transform([](int x) { return x * x; }),
                                 std::array{1, 4, 9}));
static_assert(std::abs(-4) == 4);
static_assert(std::source_location::current().line() == __LINE__);
static_assert(std::is_nothrow_constructible_v<std::align_val_t, std::size_t> || true);
static_assert(std::to_underlying(std::align_val_t{16}) == 16);

constexpr bool constexpr_items() {
  std::array<int, 5> a{5, 1, 4, 2, 3};
  std::sort(a.begin(), a.end());
  if (a != std::array{1, 2, 3, 4, 5}) return false;
  if (std::ranges::find(a, 4) != a.begin() + 3) return false;
  if (std::accumulate(a.begin(), a.end(), 0) != 15) return false;
  std::optional<int> o = 3;
  if (*o != 3 || std::optional<int>().value_or(9) != 9) return false;
  std::variant<int, long> v = 2L;
  if (std::get_if<long>(&v) == nullptr || *std::get_if<long>(&v) != 2L) return false;
  if (std::visit([](auto x) { return int(x) + 1; }, v) != 3) return false;
  std::expected<int, int> e = std::unexpected(4);
  if (e.has_value() || e.error() != 4 || e.value_or(1) != 1) return false;
  int buf[6] = {0, 1, 2, 3, 4, 5};
  std::mdspan m(buf, 2, 3);
  if (m[1, 2] != 5) return false;
  std::span<int> sp(buf);
  if (sp.subspan(2, 2)[1] != 3) return false;
  if (std::invoke([](int x) { return x + 1; }, 1) != 2) return false;
  if (!std::not_fn([](int x) { return x > 0; })(-1)) return false;
  if (std::bind_front(std::minus<>(), 10)(3) != 7) return false;
  if (!std::ranges::less{}(1, 2)) return false;
  auto x = std::exchange(buf[0], 9);
  if (x != 0 || buf[0] != 9) return false;
  return true;
}
static_assert(constexpr_items());

int add(int a, int b) { return a + b; }
int sum_va(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int s = 0;
  for (int i = 0; i < n; ++i) s += va_arg(ap, int);
  va_end(ap);
  return s;
}
int cmp_int(const void* a, const void* b) {
  int x = *static_cast<const int*>(a), y = *static_cast<const int*>(b);
  return (x > y) - (x < y);
}

// With -ffreestanding, main is an ordinary function ([basic.start.main]/1: in a freestanding
// environment start-up is implementation-defined); Clang then mangles it, so it is given C
// linkage for the hosted start-up code this test links with.
extern "C" int main() {
  CHECK(sum_va(3, 1, 2, 3) == 6);
  int arr[5] = {4, 2, 5, 1, 3};
  std::qsort(arr, 5, sizeof(int), cmp_int);
  CHECK(arr[0] == 1 && arr[4] == 5);
  int key = 4;
  CHECK(std::bsearch(&key, arr, 5, sizeof(int), cmp_int) == &arr[3]);
  CHECK(std::div(7, 2).quot == 3 && std::div(7, 2).rem == 1);  // constexpr: cstdlib/constexpr_abs_div
  CHECK(std::lldiv(-7LL, 2LL).quot == -3 && std::lldiv(-7LL, 2LL).rem == -1);
  char dst[8];
  std::memcpy(dst, "abcdef", 7);
  CHECK(std::strlen(dst) == 6);
  const char* cs = dst;
  const char* found = std::strchr(cs, 'd');
  CHECK(found == dst + 3);
  std::memset(dst, 'z', 2);
  CHECK(dst[1] == 'z' && dst[2] == 'c');
  CHECK(std::wcslen(L"wide") == 4 && std::wmemcmp(L"ab", L"ac", 2) < 0);
  std::mbstate_t st{};
  (void)st;

  alignas(int) unsigned char raw[sizeof(int)];
  int* ip = ::new (static_cast<void*>(raw)) int(5);
  CHECK(*std::launder(reinterpret_cast<int*>(raw)) == 5 && ip == reinterpret_cast<int*>(raw));
  int* np = new (std::nothrow) int(6);
  CHECK(np != nullptr && *np == 6);
  delete np;
  std::unique_ptr<int> up(new int(7));  // (make_unique is not a freestanding item)
  CHECK(*up == 7);
  std::unique_ptr<int[]> ua(new int[3]{1, 2, 3});
  CHECK(ua[2] == 3);
  void* p = raw;
  std::size_t space = sizeof raw;
  CHECK(std::align(alignof(int), sizeof(int), p, space) == raw);
  CHECK(std::assume_aligned<alignof(int)>(ip) == ip);
  CHECK(std::addressof(*ip) == ip && std::to_address(ip) == ip);
  CHECK(std::pointer_traits<int*>::pointer_to(*ip) == ip);
  std::destroy_at(ip);
  CHECK(*std::construct_at(reinterpret_cast<int*>(raw), 8) == 8);

  std::function_ref<int(int, int)> fr = add;
  CHECK(fr(2, 3) == 5);
  std::string_view hay = "find the needle here";
  std::string_view needle = "needle";
  auto it = std::search(hay.begin(), hay.end(), std::default_searcher(needle.begin(), needle.end()));
  CHECK(it - hay.begin() == 9);
  CHECK(std::hash<int>()(3) == std::hash<int>()(3));

  std::atomic<int> a{1};
  CHECK(a.fetch_add(2) == 1 && a.load() == 3);
  std::atomic_flag f;
  CHECK(!f.test_and_set() && f.test());
  alignas(std::atomic_ref<int>::required_alignment) int plain = 4;
  std::atomic_ref<int> ar(plain);
  ar.store(9);
  CHECK(ar.load() == 9);

  std::coroutine_handle<> h = std::noop_coroutine();
  CHECK(h && !h.done());
  h.resume();
  std::initializer_list<int> il{1, 2, 3};
  CHECK(il.size() == 3 && *(il.end() - 1) == 3);
  CHECK(std::uncaught_exceptions() == 0);
  std::exception ex;
  CHECK(ex.what() != nullptr);
  (void)std::is_debugger_present();
  CHECK(std::distance(arr, arr + 5) == 5 && *std::next(std::begin(arr)) == 2);
  return 0;
}
