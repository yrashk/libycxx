// libycxx: the static mode's link-time marker (DECISIONS §20.2). Every translation unit compiled
// in static mode refers to it (ycxx/config.hpp), so a static-mode object linked in shared mode
// fails to link, naming it. Defined in the static archives (libycxx.a, libycxx-freestanding.a),
// hidden.
#include <ycxx/config.hpp>

extern "C" [[__gnu__::__visibility__("hidden")]] constinit const char __ycxx_linkage_static_v1 = 0;
