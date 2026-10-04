// Third translation unit of linkage/static_destruction_iostreams.pass.cpp. It does NOT include
// <iostream> (nor any header that is required to declare the standard iostream objects), so no
// ios_base::Init object of its own is ordered before c3. c3's destructor still writes to cout
// (through tu1_say): the objects "are not destroyed during program execution"
// ([iostream.objects.overview]/3 and its footnote: destructors of objects with static storage
// duration can write output to stdout). In the default synchronized mode that output goes to the
// C stream stdout, which exit flushes after all static objects are destroyed
// ([support.start.term]/9.2), so it must appear.
#include <string>
#include "child_process.hpp"

void tu1_say(const std::string& s);

namespace {
struct Quiet3 {
  std::string name = "c3";
  ~Quiet3() {
    if (child_mode() && !child_mode_is("unsync")) tu1_say("dtor " + name + " ok");
  }
};
Quiet3 c3;
}  // namespace

int tu3_touch() { return static_cast<int>(c3.name.size()); }
