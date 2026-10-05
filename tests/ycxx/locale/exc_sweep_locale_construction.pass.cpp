// Constructing, combining and installing locales while operator new fails at its k-th call,
// for every k until the operation completes. No guarantee beyond the basic one is stated for
// these members, so each run checks that nothing is leaked, that the exception is a
// std::exception ([res.on.exception.handling]/4, footnote 147: an allocation failure is
// reported by bad_alloc), and that every locale object involved is still usable and unchanged
// when it was only read:
//   [locale.cons]: locale(const char*), locale(const string&), locale(const locale&, const
//     char*, category), locale(const locale&, const locale&, category), locale(const locale&,
//     Facet*) (with a facet whose refs is 1, so that the program keeps ownership whatever
//     happens); [locale.members] combine<Facet>, name(); [locale.operators] operator==;
//   [locale.statics]/2 global: when it throws, nothing says the global locale changed: it is
//     checked to be one of the old or the new one, and usable;
//   [ios.base.locales]/1-2 imbue: "Postconditions: loc == getloc()" when it returns.
// The named locale "C.UTF-8" is used if the system provides it, else only "C".
#include <locale>
#include <sstream>
#include <string>
#include "exc_new.hpp"

using namespace exh;

struct Tag : std::locale::facet {
  static inline std::locale::id id;
  Tag() : std::locale::facet(1) {}  // refs 1: never deleted by a locale
};

template <class F>
void sw(const char* name, F op) {
  sweep_new(name, [&] {
    bool other = false;
    bool threw = attempt([&] {
      try {
        op();
      } catch (const alloc_failure&) {
        throw;
      } catch (const std::exception&) {
        other = true;  // e.g. runtime_error from a named-locale constructor: acceptable
        throw alloc_failure(gnew);
      }
    });
    (void)other;
    return threw;
  });
}

static bool usable(const std::locale& l) {
  disarm();
  std::ostringstream os;
  os.imbue(l);
  os << 12345 << ' ' << 1.5;
  return os.str() == "12345 1.5" && std::use_facet<std::ctype<char>>(l).toupper('q') == 'Q';
}

int main() {
  std::string named = "C";
  try {
    std::locale probe("C.UTF-8");
    named = "C.UTF-8";
  } catch (const std::runtime_error&) {
  }
  static Tag tag;
  const std::locale classic = std::locale::classic();
  const std::locale base(std::locale::classic(), &tag);

  sw("locale(const char*)", [&] {
    std::locale l(named.c_str());
    EXH_EXPECT(l.name() == named && usable(l), "locale(name)");
  });
  sw("locale(const string&)", [&] {
    std::locale l(named);
    EXH_EXPECT(usable(l), "locale(string)");
  });
  sw("locale(loc, name, category)", [&] {
    std::locale l(base, named.c_str(), std::locale::numeric | std::locale::ctype);
    EXH_EXPECT(std::has_facet<Tag>(l) && usable(l), "locale(loc, name, cat)");
    EXH_EXPECT(usable(base), "the source changed");
  });
  sw("locale(loc, loc, category)", [&] {
    std::locale l(classic, base, std::locale::all);
    EXH_EXPECT(usable(l), "locale(loc, loc, cat)");
  });
  sw("locale(loc, Facet*)", [&] {
    std::locale l(classic, &tag);
    EXH_EXPECT(std::has_facet<Tag>(l) && &std::use_facet<Tag>(l) == &tag && usable(l), "locale(loc, f)");
  });
  sw("combine", [&] {
    std::locale l = classic.combine<Tag>(base);
    EXH_EXPECT(std::has_facet<Tag>(l) && usable(l), "combine");
    EXH_EXPECT(!std::has_facet<Tag>(classic), "combine changed *this");
  });
  sw("name and ==", [&] {
    std::locale l(base);
    std::string n = l.name();
    EXH_EXPECT(n == "*" && l == base && !(l == classic), "name/==");
  });
  sw("global", [&] {
    std::locale old;
    bool set = false;
    try {
      std::locale::global(base);
      set = true;
    } catch (...) {
      disarm();
      std::locale now;
      EXH_EXPECT(now == old || now == base, "a failed global() left another locale installed");
      EXH_EXPECT(usable(now), "the global locale is unusable");
      std::locale::global(old);
      throw;
    }
    if (set) {
      disarm();
      EXH_EXPECT(std::locale() == base, "global");
      std::locale::global(old);
    }
  });
  sw("imbue", [&] {
    disarm();
    std::stringstream s;
    st.left[gnew] = st.k;
    std::locale prev = s.imbue(base);
    EXH_EXPECT(s.getloc() == base && prev == std::locale(), "imbue");
  });
  return finish();
}
