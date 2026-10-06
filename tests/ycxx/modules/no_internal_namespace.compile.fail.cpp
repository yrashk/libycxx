// [std.modules]/6 (recommended practice): the modules export nothing but the standard's
// declarations. libycxx's implementation namespace ycxx (ycxx::detail, ycxx::adl_free), which
// the headers in the module's global module fragment declare, is not visible after `import std;`.
// (Clang 23 may find the declaration and report it not visible: "must be declared before it is
// used ... declaration here is not visible"; either way the name cannot be used.)
// MODULES: std
// EXPECT-ERROR-CLANG: use of undeclared identifier 'ycxx'|'hardened' must be declared before it is used
// EXPECT-ERROR-GCC: .ycxx. has not been declared
import std;

bool internal = ycxx::detail::cfg::hardened;
