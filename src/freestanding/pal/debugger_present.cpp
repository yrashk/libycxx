// libycxx freestanding runtime: the default PAL debugger query, which answers false. A
// freestanding program can define ycxx_pal_debugger_present (or std::is_debugger_present).
#include <ycxx/pal.h>

extern "C" int ycxx_pal_debugger_present() noexcept { return 0; }
