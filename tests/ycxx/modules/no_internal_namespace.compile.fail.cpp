// [std.modules]/6 (recommended practice): the modules export nothing but the standard's
// declarations. libycxx's implementation namespace __ycxx (__ycxx::__detail, __ycxx::__adl_free), which
// the headers in the module's global module fragment declare, is not visible after `import std;`.
// (Clang 23 may find the declaration and report it not visible: "must be declared before it is
// used ... declaration here is not visible"; either way the name cannot be used.)
// MODULES: std
// EXPECT-ERROR-CLANG: use of undeclared identifier '__ycxx'|'__hardened' must be declared before it is used
// EXPECT-ERROR-GCC: .__ycxx. has not been declared
import std;

bool internal = __ycxx::__detail::__cfg::__hardened;
