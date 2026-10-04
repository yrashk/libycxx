// [bit.cast]/1, [bit.byteswap]/1, [bit.pow.two], [bit.shift]/2,4, [bit.rotate]/2,5,
// [bit.count], [bit.permute]: "Constraints: T is an unsigned integer type
// ([basic.fundamental])" (or "Each of T and S is a signed or unsigned integer type" for
// shl/shr, "T models integral" for byteswap). Character types, bool, enumerations,
// std::byte and floating-point types are not (un)signed integer types, so the calls do not
// participate in overload resolution.
#include <bit>
#include <cstddef>
#include <cstdint>

enum E : unsigned { e0 };
enum class SE : unsigned { e0 };

template <class T> concept HasSingleBit = requires(T t) { std::has_single_bit(t); };
template <class T> concept BitCeil = requires(T t) { std::bit_ceil(t); };
template <class T> concept BitFloor = requires(T t) { std::bit_floor(t); };
template <class T> concept BitWidth = requires(T t) { std::bit_width(t); };
template <class T> concept Rotl = requires(T t) { std::rotl(t, 1); };
template <class T> concept Rotr = requires(T t) { std::rotr(t, 1); };
template <class T> concept CountlZero = requires(T t) { std::countl_zero(t); };
template <class T> concept CountlOne = requires(T t) { std::countl_one(t); };
template <class T> concept CountrZero = requires(T t) { std::countr_zero(t); };
template <class T> concept CountrOne = requires(T t) { std::countr_one(t); };
template <class T> concept Popcount = requires(T t) { std::popcount(t); };
template <class T> concept BitReverse = requires(T t) { std::bit_reverse(t); };
template <class T> concept BitRepeat = requires(T t) { std::bit_repeat(t, 1); };
template <class T> concept BitCompress = requires(T t) { std::bit_compress(t, t); };
template <class T> concept BitExpand = requires(T t) { std::bit_expand(t, t); };
template <class T> concept Byteswap = requires(T t) { std::byteswap(t); };
template <class T, class S> concept Shl = requires(T t, S s) { std::shl(t, s); };
template <class T, class S> concept Shr = requires(T t, S s) { std::shr(t, s); };

template <class T> constexpr bool all_unsigned_ops =
    HasSingleBit<T> && BitCeil<T> && BitFloor<T> && BitWidth<T> && Rotl<T> && Rotr<T> &&
    CountlZero<T> && CountlOne<T> && CountrZero<T> && CountrOne<T> && Popcount<T> &&
    BitReverse<T> && BitRepeat<T> && BitCompress<T> && BitExpand<T>;
template <class T> constexpr bool no_unsigned_ops =
    !HasSingleBit<T> && !BitCeil<T> && !BitFloor<T> && !BitWidth<T> && !Rotl<T> && !Rotr<T> &&
    !CountlZero<T> && !CountlOne<T> && !CountrZero<T> && !CountrOne<T> && !Popcount<T> &&
    !BitReverse<T> && !BitRepeat<T> && !BitCompress<T> && !BitExpand<T>;

static_assert(all_unsigned_ops<unsigned char>);
static_assert(all_unsigned_ops<unsigned short>);
static_assert(all_unsigned_ops<unsigned int>);
static_assert(all_unsigned_ops<unsigned long>);
static_assert(all_unsigned_ops<unsigned long long>);
static_assert(all_unsigned_ops<std::uint8_t>);
static_assert(all_unsigned_ops<std::uintmax_t>);

static_assert(no_unsigned_ops<signed char>);
static_assert(no_unsigned_ops<short>);
static_assert(no_unsigned_ops<int>);
static_assert(no_unsigned_ops<long>);
static_assert(no_unsigned_ops<long long>);
static_assert(no_unsigned_ops<bool>);
static_assert(no_unsigned_ops<char>);
static_assert(no_unsigned_ops<wchar_t>);
static_assert(no_unsigned_ops<char8_t>);
static_assert(no_unsigned_ops<char16_t>);
static_assert(no_unsigned_ops<char32_t>);
static_assert(no_unsigned_ops<std::byte>);
static_assert(no_unsigned_ops<E>);
static_assert(no_unsigned_ops<SE>);
static_assert(no_unsigned_ops<float>);
static_assert(no_unsigned_ops<double>);
static_assert(no_unsigned_ops<unsigned*>);

// shl/shr: each of T and S is a signed or unsigned integer type.
static_assert(Shl<int, int> && Shr<int, int>);
static_assert(Shl<unsigned char, long long> && Shr<long long, unsigned char>);
static_assert(Shl<signed char, unsigned long long> && Shr<unsigned long long, signed char>);
static_assert(!Shl<bool, int> && !Shr<bool, int>);
static_assert(!Shl<int, bool> && !Shr<int, bool>);
static_assert(!Shl<char, int> && !Shr<char, int>);
static_assert(!Shl<int, char> && !Shr<int, char>);
static_assert(!Shl<char8_t, int> && !Shr<int, char8_t>);
static_assert(!Shl<wchar_t, int> && !Shr<int, char32_t>);
static_assert(!Shl<E, int> && !Shr<int, E>);
static_assert(!Shl<std::byte, int> && !Shr<int, std::byte>);
static_assert(!Shl<double, int> && !Shr<int, double>);

// byteswap: T models integral (so signed types, bool and character types are fine).
static_assert(Byteswap<int> && Byteswap<unsigned> && Byteswap<signed char> && Byteswap<char>);
static_assert(Byteswap<char16_t> && Byteswap<char32_t> && Byteswap<wchar_t> && Byteswap<long long>);
static_assert(!Byteswap<float> && !Byteswap<double> && !Byteswap<E> && !Byteswap<SE>);
static_assert(!Byteswap<std::byte> && !Byteswap<int*>);

// bit_compress/bit_expand take two parameters of the same T: mixed types do not deduce.
template <class T, class U> concept CompressMixed = requires(T t, U u) { std::bit_compress(t, u); };
template <class T, class U> concept ExpandMixed = requires(T t, U u) { std::bit_expand(t, u); };
static_assert(!CompressMixed<unsigned, unsigned long>);
static_assert(!ExpandMixed<unsigned char, unsigned>);

// bit_cast: sizeof(To) == sizeof(From), both trivially copyable.
struct NotTC { NotTC(const NotTC&); int x; };
template <class To, class From> concept BitCastable = requires(const From& f) { std::bit_cast<To>(f); };
static_assert(BitCastable<std::uint32_t, float>);
static_assert(BitCastable<double, std::uint64_t>);
static_assert(!BitCastable<std::uint64_t, float>);
static_assert(!BitCastable<std::uint8_t, std::uint16_t>);
static_assert(!BitCastable<NotTC, int>);
static_assert(!BitCastable<int, NotTC>);

// Return types.
static_assert(__is_same(decltype(std::bit_width(0u)), int));
static_assert(__is_same(decltype(std::popcount(std::uint8_t(0))), int));
static_assert(__is_same(decltype(std::countl_zero(0ull)), int));
static_assert(__is_same(decltype(std::bit_ceil(std::uint16_t(0))), std::uint16_t));
static_assert(__is_same(decltype(std::bit_floor(std::uint8_t(0))), std::uint8_t));
static_assert(__is_same(decltype(std::has_single_bit(0ul)), bool));
static_assert(__is_same(decltype(std::rotl(std::uint8_t(0), 1)), std::uint8_t));
static_assert(__is_same(decltype(std::bit_reverse(std::uint16_t(0))), std::uint16_t));
static_assert(__is_same(decltype(std::bit_repeat(std::uint8_t(1), 1)), std::uint8_t));
static_assert(__is_same(decltype(std::bit_compress(std::uint8_t(1), std::uint8_t(1))), std::uint8_t));
static_assert(__is_same(decltype(std::byteswap(short(1))), short));

// noexcept specifications from [bit.syn].
static_assert(noexcept(std::has_single_bit(1u)) && noexcept(std::bit_floor(1u)) && noexcept(std::bit_width(1u)));
static_assert(noexcept(std::rotl(1u, 1)) && noexcept(std::rotr(1u, 1)) && noexcept(std::popcount(1u)));
static_assert(noexcept(std::countl_zero(1u)) && noexcept(std::countr_one(1u)));
static_assert(noexcept(std::bit_reverse(1u)) && noexcept(std::bit_compress(1u, 1u)) && noexcept(std::bit_expand(1u, 1u)));
static_assert(noexcept(std::byteswap(1)) && noexcept(std::bit_cast<float>(1u)));
