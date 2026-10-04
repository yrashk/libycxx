// [template.bitset.general], [bitset.syn]: noexcept and constexpr-ness of the bitset interface.
// Members that can throw (set(pos), reset(pos), flip(pos), test(pos), to_ulong, to_ullong)
// are not noexcept; everything else listed with noexcept is.
#include <bitset>
#include <cstddef>
#include <type_traits>
#include <utility>

using B = std::bitset<40>;
B& m() noexcept;
const B& c() noexcept;

static_assert(noexcept(B()));
static_assert(noexcept(B(1ull)));
static_assert(noexcept(m() &= c()));
static_assert(noexcept(m() |= c()));
static_assert(noexcept(m() ^= c()));
static_assert(noexcept(m() <<= 1));
static_assert(noexcept(m() >>= 1));
static_assert(noexcept(c() << 1));
static_assert(noexcept(c() >> 1));
static_assert(noexcept(m().set()));
static_assert(noexcept(m().reset()));
static_assert(noexcept(~c()));
static_assert(noexcept(m().flip()));
static_assert(noexcept(c().count()));
static_assert(noexcept(c().size()));
static_assert(noexcept(c() == c()));
static_assert(noexcept(c().all()));
static_assert(noexcept(c().any()));
static_assert(noexcept(c().none()));
static_assert(noexcept(c() & c()));
static_assert(noexcept(c() | c()));
static_assert(noexcept(c() ^ c()));

// size() is usable in a constant expression
static_assert(B().size() == 40);
static_assert(std::bitset<0>().size() == 0);

// return types
static_assert(std::is_same_v<decltype(m().set(0)), B&>);
static_assert(std::is_same_v<decltype(m().reset(0)), B&>);
static_assert(std::is_same_v<decltype(m().flip(0)), B&>);
static_assert(std::is_same_v<decltype(c().test(0)), bool>);
static_assert(std::is_same_v<decltype(c().to_ulong()), unsigned long>);
static_assert(std::is_same_v<decltype(c().to_ullong()), unsigned long long>);
static_assert(std::is_same_v<decltype(c() << 1), B>);
static_assert(std::is_same_v<decltype(~c()), B>);
static_assert(std::is_same_v<decltype(m() <<= 1), B&>);
static_assert(std::is_same_v<decltype(c().all()), bool>);
