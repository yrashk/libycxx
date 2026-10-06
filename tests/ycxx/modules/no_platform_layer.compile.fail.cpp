// [std.modules]/6: the platform layer's C functions (ycxx_pal_*, declared at global scope by
// <ycxx/pal.h> in the module's global module fragment) are not visible after `import std;`.
// (Clang 23 may find the declaration and report it not visible: "must be declared before it is
// used ... declaration here is not visible"; either way the name cannot be used.)
// MODULES: std
// EXPECT-ERROR-CLANG: use of undeclared identifier 'ycxx_pal_abort'|'ycxx_pal_abort' must be declared before it is used
// EXPECT-ERROR-GCC: .ycxx_pal_abort. was not declared in this scope
import std;

void f() { ycxx_pal_abort("internal"); }
