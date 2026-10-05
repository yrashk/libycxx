// [std.modules]/6: an inline namespace of the implementation inside std (std::ranges::cpo, which
// holds the customization point objects) is not exported; its members are, as members of
// std::ranges ([range.access]).
// MODULES: std
// EXPECT-ERROR-CLANG: no member named 'cpo' in namespace 'std::ranges'|'begin' must be declared before it is used
// EXPECT-ERROR-GCC: 'cpo' is not a member of 'std::ranges'
import std;

int a[2];
auto ok = std::ranges::begin(a); // the CPO itself is exported
auto bad = std::ranges::cpo::begin(a);
