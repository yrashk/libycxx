// [std.modules]/2-3: std exports the C library's names in namespace std only; the global names
// are std.compat's (import_std_compat.pass.cpp). The C library's own declarations in std's global
// module fragment stay invisible to an importer of std.
// MODULES: std
// EXPECT-ERROR-CLANG: use of undeclared identifier 'strlen'
// EXPECT-ERROR-GCC: 'strlen' was not declared in this scope
import std;

std::size_t ok = std::strlen("ok");
std::size_t bad = strlen("bad");
