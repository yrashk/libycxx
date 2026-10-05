// The messages facet ([category.messages]).
//   [locale.category]/2 (Table "Locale category facets"): messages<char> and messages<wchar_t>
//     are in every locale; [locale.messages.byname]: messages_byname<C>("C") is constructible.
//   [locale.messages.members]: open, get and close return/call do_open, do_get, do_close.
//   [locale.messages.virtuals]/2: do_open "Returns a value less than 0 if no such catalog can
//     be opened"; /5: do_get "If no such message can be found, returns dfault"; do_close
//     releases the catalog.
// A catalog name that cannot exist is used for the failure case; if the implementation's mapping
// does open some catalog for an ordinary name, an unknown message in it yields dfault.
#include <locale>
#include <string>
#include "check.hpp"

template <class C>
struct Logged : std::messages<C> {
  mutable std::string log;
  using typename std::messages<C>::catalog;
  using typename std::messages<C>::string_type;
  explicit Logged(std::size_t refs = 0) : std::messages<C>(refs) {}

protected:
  catalog do_open(const std::string& name, const std::locale&) const override {
    log += "open:" + name + ";";
    return name == "good" ? 7 : -1;
  }
  string_type do_get(catalog cat, int set, int id, const string_type& dfault) const override {
    log += "get:" + std::to_string(cat) + "," + std::to_string(set) + "," + std::to_string(id) + ";";
    return id == 1 ? string_type(3, C('m')) : dfault;
  }
  void do_close(catalog cat) const override { log += "close:" + std::to_string(cat) + ";"; }
};

template <class C>
void standard() {
  std::locale loc = std::locale::classic();
  CHECK(std::has_facet<std::messages<C>>(loc));
  const std::messages<C>& m = std::use_facet<std::messages<C>>(loc);
  std::basic_string<C> dflt(2, C('d'));
  CHECK(m.open("/this/catalog/does/not/exist/ycxx-no-such-catalog", loc) < 0);
  auto cat = m.open("ycxx-test-catalog", loc);
  if (cat >= 0) {
    CHECK(m.get(cat, 12345, 67890, dflt) == dflt);
    m.close(cat);
  }
  std::locale by_loc(std::locale::classic(), new std::messages_byname<C>("C"));
  const std::messages<C>& by = std::use_facet<std::messages<C>>(by_loc);
  CHECK(by.open("/this/catalog/does/not/exist/ycxx-no-such-catalog", loc) < 0);
  std::locale named(std::locale::classic(), new std::messages_byname<C>("C"));
  CHECK(std::has_facet<std::messages<C>>(named));
}

template <class C>
void derived() {
  auto* f = new Logged<C>(1);
  {
    std::locale loc(std::locale::classic(), static_cast<std::messages<C>*>(f));
    const std::messages<C>& m = std::use_facet<std::messages<C>>(loc);
    CHECK(&m == f);
    CHECK(m.open("bad", loc) < 0);
    auto cat = m.open("good", loc);
    CHECK(cat == 7);
    CHECK(m.get(cat, 2, 1, std::basic_string<C>(1, C('x'))) == std::basic_string<C>(3, C('m')));
    CHECK(m.get(cat, 2, 5, std::basic_string<C>(1, C('x'))) == std::basic_string<C>(1, C('x')));
    m.close(cat);
    CHECK(f->log == "open:bad;open:good;get:7,2,1;get:7,2,5;close:7;");
  }
  delete f;  // refs 1: the locales did not own it
}

int main() {
  standard<char>();
  standard<wchar_t>();
  derived<char>();
  derived<wchar_t>();
  return 0;
}
