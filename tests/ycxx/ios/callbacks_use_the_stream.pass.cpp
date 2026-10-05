// ios_base callbacks that use the stream they are called for, and storage indices far beyond
// any small fixed table.
//   [ios.base.callback]/2: registered functions are called "in opposite order of registration.
//     Functions registered while a callback function is active are not called until the next
//     event." Here one callback registers a thousand more during an event: none of them runs
//     during that event, all of them run (newest first) at the next one.
//   [ios.base.locales]/1: during imbue, getloc() called from a callback returns the new locale,
//     so formatted output written to the stream from the callback uses the new locale's
//     numpunct ([facet.num.put.virtuals]); /2 Postconditions: loc == getloc().
//   [ios.base.storage]/1 xalloc: "Returns: index ++" (no upper bound short of int); /4 iword
//     "extends the array ... as necessary to include the element iarray[idx]", /7 pword
//     likewise; the values are retained: "until the next call to copyfmt, calling iword with the
//     same index yields another reference to the same value". iword/pword called from inside a
//     callback, with indices that make the arrays grow, keep that guarantee.
//   [basic.ios.members]/16: copyfmt calls (erase_event, *this) callbacks, copies the iword/pword
//     contents (/16.2.2), then calls the copied callbacks with copyfmt_event. A callback of
//     stream a that calls b.copyfmt(a) from inside a's imbue event copies a's current state.
//   [ios.base.cons]/2: ~ios_base calls the callbacks with erase_event; the callback still reads
//     the stream's iword/pword values then.
#include <ios>
#include <locale>
#include <sstream>
#include <string>
#include <vector>
#include "check.hpp"

struct Thousands : std::numpunct<char> {
  char do_thousands_sep() const override { return '\''; }
  std::string do_grouping() const override { return "\3"; }
};

static std::vector<int> late_calls;
static void late(std::ios_base::event ev, std::ios_base&, int idx) {
  if (ev == std::ios_base::imbue_event) late_calls.push_back(idx);
}

static int big_index = 0;  // set by main: an index after 100000 xalloc calls
static int registrations = 0;
static int marker = 0;

// Registers 1000 callbacks during its first imbue event, makes the storage arrays grow, and
// writes a number to the stream with the locale being imbued.
static void registrar(std::ios_base::event ev, std::ios_base& s, int idx) {
  if (ev != std::ios_base::imbue_event) return;
  auto& os = dynamic_cast<std::ostream&>(s);
  os << 1234567 << ';';
  ++s.iword(idx);
  s.pword(big_index) = &marker;
  s.iword(big_index) += 10;
  if (registrations++ == 0)
    for (int i = 0; i < 1000; ++i) s.register_callback(late, i);
}

static std::ostringstream* copy_target = nullptr;
static int copier_events[3] = {};
static void copier(std::ios_base::event ev, std::ios_base& s, int idx) {
  ++copier_events[ev == std::ios_base::erase_event ? 0 : ev == std::ios_base::imbue_event ? 1 : 2];
  if (ev == std::ios_base::imbue_event && copy_target && &s != copy_target) {
    std::ostringstream& t = *copy_target;
    copy_target = nullptr;
    t.copyfmt(dynamic_cast<std::ostringstream&>(s));
    t.iword(idx) += 100;  // t's own copy
  }
}

static long erase_seen_iword = -1;
static void* erase_seen_pword = nullptr;
static void eraser(std::ios_base::event ev, std::ios_base& s, int idx) {
  if (ev == std::ios_base::erase_event) {
    erase_seen_iword = s.iword(idx);
    erase_seen_pword = s.pword(big_index);
  }
}

int main() {
  const int first = std::ios_base::xalloc();
  for (int i = 0; i < 100000; ++i) big_index = std::ios_base::xalloc();
  CHECK(big_index == first + 100000);

  const std::locale grouped(std::locale::classic(), new Thousands);
  {
    std::ostringstream s;
    s.iword(first) = 5;
    s.register_callback(registrar, first);
    s.imbue(grouped);
    CHECK(s.str() == "1'234'567;");
    CHECK(late_calls.empty());  // registered during the event
    CHECK(s.getloc() == grouped);
    CHECK(s.iword(first) == 6);
    CHECK(s.iword(big_index) == 10);
    CHECK(s.pword(big_index) == &marker);

    s.imbue(std::locale::classic());
    CHECK(s.str() == "1'234'567;1234567;");
    CHECK(late_calls.size() == 1000);
    for (int i = 0; i < 1000; ++i) CHECK(late_calls[static_cast<std::size_t>(i)] == 999 - i);
    CHECK(s.iword(first) == 7);
    CHECK(s.iword(big_index) == 20);
    // Growth by plain calls after all that: earlier values stay.
    for (int k = first; k <= big_index; k += 997) s.iword(k) += k;
    CHECK(s.iword(first) == 7 + first);
    CHECK(s.pword(big_index) == &marker);
  }

  // copyfmt from inside a callback onto another stream.
  {
    std::ostringstream a, b;
    a.iword(big_index) = 42;
    a.pword(first) = &a;
    a.flags(std::ios_base::dec | std::ios_base::showpos);
    a.register_callback(copier, big_index);
    copy_target = &b;
    a.imbue(grouped);
    CHECK(copy_target == nullptr);
    CHECK(copier_events[1] == 1);  // a's imbue event only; b's copy of the pair got copyfmt_event
    CHECK(copier_events[2] == 1);
    CHECK(b.flags() == (std::ios_base::dec | std::ios_base::showpos));
    CHECK(b.getloc() == grouped);  // a's locale was already the new one ([ios.base.locales]/1)
    CHECK(b.iword(big_index) == 142);
    CHECK(b.pword(first) == &a);
    CHECK(a.iword(big_index) == 42);
    b << 123456;
    CHECK(b.str() == "+123'456");
  }
  CHECK(copier_events[0] == 2);  // a and b destroyed: one erase_event each

  // The destructor's erase_event reads the storage.
  {
    std::stringstream s;
    s.iword(big_index - 1) = 77;
    s.pword(big_index) = &erase_seen_iword;
    s.register_callback(eraser, big_index - 1);
  }
  CHECK(erase_seen_iword == 77);
  CHECK(erase_seen_pword == &erase_seen_iword);
  return 0;
}
