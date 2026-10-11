// libycxx runtime: the default std::is_debugger_present ([debugging.utility]/3-5). Replaceable,
// so it is alone in its archive member (DECISIONS §3): a program's definition is linked instead.
// Hosted: the PAL asks the OS (Linux: a tracer is attached); the freestanding runtime's default
// PAL hook answers false.
// Weak: in shared mode the library itself calls it (breakpoint_if_debugging) and exports it, and
// a program's definition must still win ([replacement.functions]). On ELF a call through the PLT
// is interposed by the program's definition either way; on Mach-O a dylib's call to its own
// non-weak function is bound when the dylib is linked (two-level namespace), whereas a weak
// definition is called through a weak binding that dyld resolves, a program's non-weak definition
// first, as with libc++abi's operator new (DECISIONS §20.8). In the archives it changes nothing:
// the member is linked only when the program has no definition.
#include <debugging>
#include <ycxx/pal.h>

[[__gnu__::__weak__]] bool std::is_debugger_present() noexcept { return ycxx_pal_debugger_present() != 0; }
