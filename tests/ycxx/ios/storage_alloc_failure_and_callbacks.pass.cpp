// ios_base's storage functions when allocation fails, and the callback registration rules.
//   [ios.base.storage]/4-5 iword: "If the function fails (footnote: for example, because it
//     cannot allocate space) and *this is a base class subobject of a basic_ios<> object or
//     subobject, the effect is equivalent to calling basic_ios<>::setstate(badbit) on the
//     derived object (which may throw failure)." "Returns: ... On failure, a valid long&
//     initialized to 0." /7-8 pword likewise ("a valid void*& initialized to 0"). So the
//     allocation failure is not propagated as bad_alloc: either the stream gets badbit (and
//     ios_base::failure is thrown iff exceptions() includes badbit), or the call succeeds.
//     "the value of the storage referred to is retained, so that until the next call to
//     copyfmt, calling iword with the same index yields another reference to the same value":
//     values stored before a failed extension are still there. Nothing is leaked.
//   [ios.base.callback]/2: callbacks are called "in opposite order of registration. Functions
//     registered while a callback function is active are not called until the next event."
//     /3: "Identical pairs are not merged. A function registered twice will be called twice."
//   [ios.base.locales]/1: during imbue, getloc() called from the callback returns the new locale.
//   [basic.ios.members]/15-16: copyfmt calls the callbacks with erase_event before and with
//     copyfmt_event after copying; [ios.base.cons]/2: the destructor calls them with erase_event.
// REQUIRES: exceptions
#include <ios>
#include <locale>
#include <sstream>
#include <string>
#include "exc_new.hpp"

using namespace exh;

static std::string trace;

static void late(std::ios_base::event ev, std::ios_base&, int idx) {
  trace += "late" + std::to_string(idx) + (ev == std::ios_base::imbue_event ? "i " : ev == std::ios_base::erase_event ? "e " : "c ");
}
static void noop(std::ios_base::event, std::ios_base&, int) {}
static bool registered_late = false;
static void early(std::ios_base::event ev, std::ios_base& s, int idx) {
  trace += "early" + std::to_string(idx) + (ev == std::ios_base::imbue_event ? "i " : ev == std::ios_base::erase_event ? "e " : "c ");
  if (ev == std::ios_base::imbue_event) {
    if (s.getloc() != std::locale::classic()) trace += "WRONG-LOCALE ";
    if (!registered_late) {
      registered_late = true;
      s.register_callback(late, 9);
    }
  }
}

template <bool Pword>
void storage_sweep(std::ios_base::iostate exc) {
  sweep_new(Pword ? "pword" : "iword", [&] {
    std::stringstream s;
    s.exceptions(exc);
    const int small = std::ios_base::xalloc();
    if constexpr (Pword)
      s.pword(small) = &s;
    else
      s.iword(small) = 77;
    int big = 0;
    for (int i = 0; i < 200; ++i) big = std::ios_base::xalloc();  // an index past the array
    bool got_failure = false, failed_value_ok = true;
    bool threw = attempt([&] {
      try {
        if constexpr (Pword) {
          void*& r = s.pword(big);
          failed_value_ok = r == nullptr;
          r = &got_failure;  // the reference is valid on failure too
        } else {
          long& r = s.iword(big);
          failed_value_ok = r == 0;
          r = 5;
        }
      } catch (const std::ios_base::failure&) {
        got_failure = true;
      }
    });
    EXH_EXPECT(!threw, "the allocation failure escaped from iword/pword (bad_alloc instead of badbit)");
    const bool failed = st.fired;
    disarm();
    if (failed) {
      EXH_EXPECT(s.rdstate() == std::ios_base::badbit, "a failed iword/pword did not set exactly badbit");
      EXH_EXPECT(got_failure == ((exc & std::ios_base::badbit) != 0), "failure thrown iff exceptions() has badbit");
      if (!got_failure) EXH_EXPECT(failed_value_ok, "the reference returned on failure does not refer to 0");
    } else {
      EXH_EXPECT(s.good() && failed_value_ok, "a successful iword/pword changed the state or the new element");
    }
    s.exceptions(std::ios_base::goodbit);
    s.clear();
    if constexpr (Pword) {
      EXH_EXPECT(s.pword(small) == &s, "a failed extension lost an earlier pword value");
      if (!failed) EXH_EXPECT(s.pword(big) == &got_failure, "pword value not retained");
    } else {
      EXH_EXPECT(s.iword(small) == 77, "a failed extension lost an earlier iword value");
      if (!failed) EXH_EXPECT(s.iword(big) == 5, "iword value not retained");
    }
    return failed;
  }, options{true, 4000});
}

int main() {
  for (auto exc : {std::ios_base::goodbit, std::ios_base::badbit, std::ios_base::failbit}) {
    storage_sweep<false>(exc);
    storage_sweep<true>(exc);
  }
  // register_callback may fail by throwing; it must not leak.
  sweep_new("register_callback", [] {
    std::stringstream s;
    for (int i = 0; i < 3; ++i) s.register_callback(noop, 100 + i);
    return attempt([&] {
      for (int i = 0; i < 40; ++i) s.register_callback(noop, i);
    });
  });

  // The registration rules.
  trace.clear();
  {
    std::stringstream s;
    s.register_callback(early, 1);
    s.register_callback(early, 1);  // not merged
    s.register_callback(late, 2);
    s.imbue(std::locale::classic());
    // Opposite order of registration; late9 (registered during the event) is not called yet,
    // and only one late9 is registered (by the first early call).
    if (trace != "late2i early1i early1i ") {
      dprintf(2, "imbue trace: %s\n", trace.c_str());
      ++st.failures;
    }
    trace.clear();
    std::stringstream t;
    t.copyfmt(s);  // erase_event on t (no callbacks yet), then copyfmt_event with s's callbacks
    if (trace != "late9c late2c early1c early1c ") {
      dprintf(2, "copyfmt trace: %s\n", trace.c_str());
      ++st.failures;
    }
    trace.clear();
  }
  // Destruction of t, then of s: erase_event for each callback of each.
  if (trace != "late9e late2e early1e early1e late9e late2e early1e early1e ") {
    dprintf(2, "destructor trace: %s\n", trace.c_str());
    ++st.failures;
  }
  return finish();
}
