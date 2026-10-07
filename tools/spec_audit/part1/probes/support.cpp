// [support]: <cstddef> ([support.types]), <limits> ([support.limits]), <new>
// ([support.dynamic]), <source_location>, <initializer_list>, <compare> ([cmp], type_order
// P2830), <coroutine>, the freestanding <cstdlib> (memalignment, constexpr abs/div). All of these
// are freestanding.
// FREESTANDING
#include <cstddef>
#include <cstdlib>
#include <compare>
#include <coroutine>
#include <initializer_list>
#include <limits>
#include <new>
#include <source_location>
#include <type_traits>

// [support.types.byteops]
static_assert(std::to_integer<int>(std::byte{3} << 1) == 6 && noexcept(std::to_integer<int>(std::byte{})));
static_assert((std::byte{6} & std::byte{3}) == std::byte{2} && (~std::byte{0}) == std::byte{255});
template <class I>
concept shiftable = requires(std::byte b, I i) { b << i; };
static_assert(!shiftable<float> && shiftable<long>);
static_assert(std::is_same_v<decltype(nullptr), std::nullptr_t> && alignof(std::max_align_t) >= alignof(long double));
struct OffS { int a; char b; };
static_assert(offsetof(OffS, b) == sizeof(int));
// [numeric.limits]
static_assert(std::numeric_limits<int>::max() == 2147483647 && noexcept(std::numeric_limits<int>::max()));
static_assert(std::numeric_limits<double>::is_iec559 && std::numeric_limits<unsigned>::is_modulo);
static_assert(std::numeric_limits<const volatile int>::digits == 31);
static_assert(std::numeric_limits<char8_t>::is_specialized && std::numeric_limits<char32_t>::digits == 32);
static_assert(std::numeric_limits<std::byte>::is_specialized == false);
static_assert(std::numeric_limits<float>::round_style == std::round_to_nearest);
// [support.dynamic]
static_assert(std::is_same_v<std::underlying_type_t<std::align_val_t>, std::size_t>);
static_assert(std::is_same_v<decltype(::operator new(1, std::nothrow)), void*> && noexcept(::operator new(1, std::nothrow)));
static_assert(std::is_same_v<decltype(::operator new(1, std::align_val_t(16))), void*>);
static_assert(noexcept(::operator delete(nullptr, 1, std::align_val_t(16))));
static_assert(std::is_trivially_copyable_v<std::destroying_delete_t> && std::is_trivially_copyable_v<std::nothrow_t>);
static_assert(std::hardware_destructive_interference_size >= alignof(std::max_align_t) &&
              std::hardware_constructive_interference_size >= alignof(std::max_align_t));
static_assert([] { int x = 1; return *std::launder(&x) == 1; }() && noexcept(std::launder(static_cast<int*>(nullptr))));
static_assert([] { int x = 1; int* p = ::new (static_cast<void*>(&x)) int(2); return *p == 2; }());   // constexpr placement new
static_assert(std::is_base_of_v<std::bad_alloc, std::bad_array_new_length>);
static_assert(std::is_same_v<decltype(std::set_new_handler(nullptr)), std::new_handler>);
// [support.srcloc]
static_assert(std::source_location::current().line() == __LINE__);
static_assert(noexcept(std::source_location()) && noexcept(std::source_location().column()));
static_assert(std::is_same_v<decltype(std::source_location().file_name()), const char*>);
constexpr std::source_location here(std::source_location l = std::source_location::current()) { return l; }
static_assert(here().line() == __LINE__);
// [support.initlist]
static_assert(std::initializer_list<int>{1, 2}.size() == 2 && *std::initializer_list<int>{1, 2}.data() == 1);
static_assert(std::initializer_list<int>().empty() && noexcept(std::initializer_list<int>().empty()));
// [cmp]
static_assert(std::is_eq(0 <=> 0) && std::is_lt(std::strong_ordering::less) && noexcept(std::is_gt(1 <=> 0)));
static_assert(std::is_same_v<std::common_comparison_category_t<std::strong_ordering, std::partial_ordering>, std::partial_ordering>);
static_assert(std::is_same_v<std::compare_three_way_result_t<int>, std::strong_ordering>);
static_assert(std::three_way_comparable_with<int, long> && !std::three_way_comparable<void*()>);
static_assert(std::strong_order(1.0, 2.0) == std::strong_ordering::less);
static_assert(std::weak_order(-0.0, 0.0) == std::weak_ordering::equivalent);
static_assert(std::is_same_v<decltype(std::partial_order(1, 2)), std::partial_ordering>);
static_assert(std::compare_partial_order_fallback(1, 2) == std::partial_ordering::less);
static_assert(noexcept(std::strong_order(1, 2)));
// [compare.type] type_order (P2830)
static_assert(std::type_order_v<int, int> == std::strong_ordering::equal);
static_assert(std::type_order_v<int, long> != std::strong_ordering::equal &&
              std::type_order_v<int, long> == 0 <=> std::type_order_v<long, int>);
static_assert(std::is_same_v<std::remove_cv_t<decltype(std::type_order<int, long>::value)>, std::strong_ordering>);
// [support.coroutine]
struct Task { struct promise_type {}; };
static_assert(std::is_same_v<std::coroutine_traits<Task, int>::promise_type, Task::promise_type>);
template <class R>
concept has_promise = requires { typename std::coroutine_traits<R>::promise_type; };
static_assert(!has_promise<int>);   // [coroutine.traits.primary]: no member unless R::promise_type
static_assert(std::is_same_v<decltype(std::coroutine_handle<Task::promise_type>::from_promise(
                                 std::declval<Task::promise_type&>())), std::coroutine_handle<Task::promise_type>>);
static_assert(std::coroutine_handle<>() == nullptr && noexcept(std::coroutine_handle<>::from_address(nullptr)));
static_assert(std::is_same_v<decltype(std::coroutine_handle<>() <=> std::coroutine_handle<>()), std::strong_ordering>);
static_assert(noexcept(std::noop_coroutine()) && std::is_same_v<decltype(std::noop_coroutine()), std::noop_coroutine_handle>);
static_assert(noexcept(std::suspend_always().await_ready()) && !std::suspend_always().await_ready() && std::suspend_never().await_ready());
static_assert(std::is_convertible_v<std::noop_coroutine_handle, std::coroutine_handle<>>);
// [cstdlib.syn] freestanding parts
static_assert(std::abs(-3) == 3 && std::abs(-3L) == 3 && std::div(7, 2).quot == 3 && std::lldiv(7, 2).rem == 1);
static_assert(std::is_same_v<decltype(std::memalignment(nullptr)), std::size_t>);
// [cstdarg.syn]: va_start(V, ...) takes the va_list alone (C23); __STDC_VERSION_STDARG_H__
#include <cstdarg>
static_assert(__STDC_VERSION_STDARG_H__ == 202311L);
int va_one(...) { std::va_list ap; va_start(ap); int x = va_arg(ap, int); va_end(ap); return x; }
int va_two(int n, ...) { std::va_list ap; va_start(ap, n); std::va_list c; va_copy(c, ap); int x = va_arg(c, int); va_end(c); va_end(ap); return x; }
