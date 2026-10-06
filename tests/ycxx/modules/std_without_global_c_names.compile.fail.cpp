// [std.modules]/2-3: std exports the C library's names in namespace std only; the global names
// are std.compat's (import_std_compat.pass.cpp). The C library's own declarations in std's global
// module fragment stay invisible to an importer of std.
// (Clang 23 may find the declaration and report it not visible: "must be declared before it is
// used ... declaration here is not visible"; either way the name cannot be used.)
// MODULES: std
// EXPECT-ERROR-CLANG: use of undeclared identifier 'strlen'|'strlen' must be declared before it is used
// EXPECT-ERROR-GCC: .strlen. was not declared in this scope
import std;

std::size_t ok = std::strlen("ok");
std::size_t bad = strlen("bad");
