// libycxx runtime: the default std::is_debugger_present ([debugging.utility]/3-5). Replaceable,
// so it is alone in its archive member (DECISIONS §3): a program's definition is linked instead.
// Hosted: the PAL asks the OS (Linux: a tracer is attached); the freestanding runtime's default
// PAL hook answers false.
#include <debugging>
#include <ycxx/pal.h>

bool std::is_debugger_present() noexcept { return __ycxx_pal_debugger_present() != 0; }
