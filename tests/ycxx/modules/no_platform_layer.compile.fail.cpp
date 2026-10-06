// [std.modules]/6: the platform layer's C functions (__ycxx_pal_*, declared at global scope by
// <ycxx/pal.h> in the module's global module fragment) are not visible after `import std;`.
// (Clang 23 may find the declaration and report it not visible: "must be declared before it is
// used ... declaration here is not visible"; either way the name cannot be used.)
// MODULES: std
// EXPECT-ERROR-CLANG: use of undeclared identifier '__ycxx_pal_abort'|'__ycxx_pal_abort' must be declared before it is used
// EXPECT-ERROR-GCC: .__ycxx_pal_abort. was not declared in this scope
import std;

void f() { __ycxx_pal_abort("internal"); }
