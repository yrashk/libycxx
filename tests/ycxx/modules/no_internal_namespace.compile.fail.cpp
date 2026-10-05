// [std.modules]/6 (recommended practice): the modules export nothing but the standard's
// declarations. libycxx's implementation namespace ycxx (ycxx::detail, ycxx::adl_free), which
// the headers in the module's global module fragment declare, is not visible after `import std;`.
// MODULES: std
// EXPECT-ERROR-CLANG: use of undeclared identifier 'ycxx'
// EXPECT-ERROR-GCC: 'ycxx' has not been declared
import std;

bool internal = ycxx::detail::cfg::hardened;
