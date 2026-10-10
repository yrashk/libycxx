// libycxx: the shared mode's link-time marker (DECISIONS §20.2). Every translation unit compiled
// in shared mode refers to it (ycxx/config.hpp), so a shared-mode object linked in static mode
// fails to link, naming it. Defined in libycxx.so, exported.
#include <ycxx/config.hpp>

extern "C" [[__gnu__::__visibility__("default")]] constinit const char __ycxx_linkage_shared_v1 = 0;
