#include <string>

// Default visibility explicitly: with GCC a function whose signature names a libycxx type is
// hidden otherwise (docs/BUILDING_PROJECTS.md, "Shared libraries").
[[gnu::visibility("default")]] std::string mgreet(const std::string& who) { return "meson " + who; }
