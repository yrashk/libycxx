// [bitset], [any]: bitset (constexpr since C++23, string_view constructor P2697, hash),
// any (any_cast forms, make_any, bad_any_cast).
#include <bitset>
#include <any>
#include <string>
#include <string_view>
#include <type_traits>

using B = std::bitset<8>;
static_assert(B(5).count() == 2 && B(5).test(2) && (B(5) << 1).to_ulong() == 10);
static_assert([] { B b; b.set(1).flip(2).reset(1); b[3] = true; return b.to_ullong() == 12 && b.any() && !b.all(); }());
static_assert(B("101").to_ulong() == 5 && B(std::string_view("110")).to_ulong() == 6);      // [bitset.cons]
static_assert(B("xyx", 3, 'x', 'y').to_ulong() == 2);
static_assert(std::is_constructible_v<B, std::string_view, std::size_t, std::size_t, char, char>);
static_assert([] { return B(3).to_string() == "00000011" && B(3).to_string('a', 'b') == "aaaaaabb"; }());
static_assert(noexcept(B().count()) && noexcept(B().size()) && noexcept(B() == B()) && noexcept(B() & B()));
static_assert(std::is_same_v<decltype(std::declval<const B&>()[0]), bool>);
static_assert(std::is_same_v<decltype(std::declval<B&>()[0]), B::reference>);
static_assert(noexcept(std::declval<B::reference&>().flip()) && noexcept(~std::declval<B::reference&>()));
static_assert(std::is_default_constructible_v<std::hash<B>>);

// [any]
static_assert(std::is_nothrow_default_constructible_v<std::any>);
static_assert(noexcept(std::declval<std::any&>().has_value()) && noexcept(std::declval<std::any&>().reset()));
static_assert(std::is_same_v<decltype(std::any_cast<int>(std::declval<std::any*>())), int*>);
static_assert(std::is_same_v<decltype(std::any_cast<int>(std::declval<const std::any*>())), const int*>);
static_assert(noexcept(std::any_cast<int>(std::declval<std::any*>())));
static_assert(std::is_same_v<decltype(std::any_cast<int&>(std::declval<std::any&>())), int&>);
static_assert(std::is_same_v<decltype(std::make_any<std::string>(3, 'x')), std::any>);
static_assert(std::is_base_of_v<std::bad_cast, std::bad_any_cast>);
struct NoCopy { NoCopy(const NoCopy&) = delete; };
static_assert(!std::is_constructible_v<std::any, NoCopy>);     // Constraints: copy constructible
static_assert(std::is_same_v<decltype(std::declval<std::any&>().emplace<int>(1)), int&>);
