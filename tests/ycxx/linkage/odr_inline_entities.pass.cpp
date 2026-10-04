// Two translation units (this one and support/linkage/odr_tu2.cpp) including the same headers:
// library entities and program inline entities of library types are single entities program-
// wide, and state set through one TU is the state the other sees.
//   [dcl.inline]/7: an inline function or variable with external linkage has the same address in
//     all translation units; its block-scope statics are one object.
//   [basic.def.odr]/15-16: a program-defined std::formatter specialization defined identically
//     in both TUs is one entity; formatting gives the same text in both ([format.formatter]).
//   [expr.typeid]/5, [type.index.members]: typeid of the same type in two TUs compares equal;
//     type_index too; hash_code equal ([type.info]/7: "hash_code ... returns the same value
//     for any two type_info objects that compare equal").
//   [unord.hash]/2 (Cpp17Hash): h(k) depends only on k for the duration of the program.
//   [syserr.errcat.objects]/1,4: "All calls to this function shall return references to the
//     same object" (generic_category, system_category).
//   [iostream.objects.overview]/2-3: one cout object; flags set in one TU are seen in the other.
//   [locale.cons]/1, [locale.statics]/1: locale() is a copy of the global locale set by
//     locale::global in any TU; [locale.members]/5: an unnamed locale's name() is "*".
//   [locale.id], [locale.facet]: a program-defined facet's static inline id is one object; a
//     locale built in one TU is searched in the other with the same result (has_facet,
//     use_facet returning the very facet object installed).
//   [ios.base.storage]/1-2: xalloc "Returns: index ++": distinct indices program-wide, also
//     when called during dynamic initialisation of different TUs; iword(i) of one stream is the
//     same storage whichever TU asks.
//   [thread.thread.this], [basic.stc.thread]: an inline thread_local variable is one object per
//     thread in all TUs.
// FLAGS: -pthread ../../../../tests/ycxx/support/linkage/odr_tu2.cpp
//   (relative to the per-test temporary directory build/lit-*/linkage/<name>.XXXX)
#include <cstring>
#include <iostream>
#include <locale>
#include <thread>
#include "linkage/odr_shared.hpp"
#include "check.hpp"

namespace {
const int tu1_index = std::ios_base::xalloc();
struct Registrar1 {
  Registrar1() {
    registry()["tu1"] = 1;
    ++shared_counter;
  }
} registrar1;

struct Comma : std::numpunct<char> {
  char do_decimal_point() const override { return ','; }
};
}  // namespace

int main() {
  CHECK(tu2_greeting() == &shared_greeting && *tu2_greeting() == "hello");
  CHECK(tu2_numbers() == &shared_numbers);
  CHECK(tu2_counter() == &shared_counter);
  shared_greeting += " world";
  shared_numbers.push_back(4);
  CHECK(*tu2_greeting() == "hello world");
  CHECK(tu2_numbers()->size() == 4 && tu2_numbers()->back() == 4);

  // Static objects of both TUs registered (both dynamic initialisations have happened: TU2's at
  // the latest before the first odr-use of a function of TU2, [basic.start.dynamic]/5).
  CHECK(tu2_registry() == &registry());
  CHECK(registry().size() == 2 && registry().at("tu1") == 1 && registry().at("tu2") == 2);
  CHECK(shared_counter.load() == 2);
  ++*tu2_counter();
  CHECK(shared_counter.load() == 3);

  // One formatter specialization.
  CHECK(tu2_format(Point{1, -2}) == "(1, -2)|[(1, -2)]");
  CHECK(std::format("{}|{}", Point{1, -2}, std::vector<Point>{Point{1, -2}}) == tu2_format(Point{1, -2}));

  // Type identity.
  CHECK(tu2_type_index() == std::type_index(typeid(std::map<std::string, std::vector<int>>)));
  CHECK(tu2_type_info() == typeid(std::vector<Point>));
  CHECK(tu2_type_info().hash_code() == typeid(std::vector<Point>).hash_code());
  CHECK(std::strcmp(tu2_type_info().name(), typeid(std::vector<Point>).name()) == 0 ||
        tu2_type_info() == typeid(std::vector<Point>));
  CHECK(tu2_hash("abc") == std::hash<std::string>{}("abc"));

  // Error categories: one object each.
  CHECK(tu2_generic_category() == &std::generic_category());
  CHECK(tu2_system_category() == &std::system_category());
  CHECK(tu2_error_message(EDOM) == std::generic_category().message(EDOM));
  std::error_code ec(EDOM, *tu2_generic_category());
  CHECK(ec == std::errc::argument_out_of_domain);
  CHECK(ec.default_error_condition() == std::error_condition(EDOM, std::generic_category()));

  // The standard stream objects.
  CHECK(tu2_cout_address() == static_cast<const void*>(&std::cout));
  std::ios_base::fmtflags old = std::cout.flags();
  std::cout << std::hex << std::showbase;
  CHECK(tu2_cout_flags() == std::cout.flags());
  CHECK((tu2_cout_flags() & std::ios_base::basefield) == std::ios_base::hex);
  std::cout.flags(old);
  CHECK(tu2_cout_flags() == old);

  // The global locale.
  CHECK(tu2_locale_name() == "C");
  std::locale prev = std::locale::global(std::locale(std::locale::classic(), new Comma));
  CHECK(prev.name() == "C");
  CHECK(tu2_locale_name() == "*");
  CHECK(std::use_facet<std::numpunct<char>>(std::locale()).decimal_point() == ',');
  std::locale::global(std::locale::classic());
  CHECK(tu2_locale_name() == "C");

  // Facets.
  std::locale g1(std::locale::classic(), new Greeter("tu1"));
  CHECK(tu2_has_greeter(g1) && !tu2_has_greeter(std::locale::classic()));
  CHECK(tu2_use_greeter(g1) == &std::use_facet<Greeter>(g1) && tu2_use_greeter(g1)->word == "tu1");
  std::locale g2 = tu2_make_greeter_locale();
  CHECK(std::has_facet<Greeter>(g2) && std::use_facet<Greeter>(g2).word == "tu2");
  std::locale g3 = std::locale::classic().combine<Greeter>(g2);
  CHECK(tu2_use_greeter(g3) == &std::use_facet<Greeter>(g2));

  // xalloc / iword.
  CHECK(tu1_index != tu2_xalloc_index());
  int third = std::ios_base::xalloc();
  CHECK(third != tu1_index && third != tu2_xalloc_index());
  std::cout.iword(tu1_index) = 11;
  tu2_iword(std::cout) = 22;
  CHECK(std::cout.iword(tu1_index) == 11 && std::cout.iword(tu2_xalloc_index()) == 22);

  // inline thread_local: one object per thread, the same in both TUs.
  CHECK(tu2_tl() == &shared_tl && *tu2_tl() == "tl");
  const std::string* main_tl = &shared_tl;
  shared_tl = "main";
  std::thread([main_tl] {
    CHECK(tu2_tl() == &shared_tl);
    CHECK(tu2_tl() != main_tl);
    CHECK(shared_tl == "tl");  // a fresh object, initialized for this thread
    shared_tl = "worker";
  }).join();
  CHECK(shared_tl == "main");
  return 0;
}
