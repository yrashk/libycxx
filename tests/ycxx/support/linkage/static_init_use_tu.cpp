// A translation unit whose static object's constructor is the first user of the library in the
// image it is linked into (a program: linkage/library_first_use_static_init.pass.cpp; a shared
// library: linkage/library_first_use_static_init_shared.pass.cpp). The standard places no
// restriction on calling library functions during dynamic initialization; the library's own
// state (global locale, error categories, default memory resource, time zone database, text
// encoding, regex traits, format) must be usable however the initialization of this
// translation unit is ordered relative to the library's own ([basic.start.dynamic]/7: the
// order across translation units is unspecified).
#include "static_init_use.hpp"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
#include <memory_resource>
#include <random>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {
struct Comma : std::numpunct<char> {
  char do_decimal_point() const override { return ','; }
};

struct FirstUser {
  StaticInitReport r;
  void check(bool ok, const char* what) {
    if (!ok) {
      ++r.failures;
      r.failed += what;
      r.failed += "; ";
    }
  }
  FirstUser() {
    // [locale.cons]/1, [locale.statics]/4-5: before any call of global(), locale() behaves as
    // classic(), the "C" locale.
    std::locale l;
    check(l == std::locale::classic(), "locale() == classic()");
    check(std::locale::classic().name() == "C", "classic().name()");
    check(std::use_facet<std::ctype<char>>(l).toupper('q') == 'Q', "ctype toupper");
    check(std::use_facet<std::numpunct<char>>(l).decimal_point() == '.', "numpunct decimal_point");
    std::ostringstream os;
    os << 1.5 << ' ' << 42;
    check(os.str() == "1.5 42", "ostringstream");
    check(std::format("{:>5}|{}|{:x}", 42, 2.5, 255) == "   42|2.5|ff", "format");
    // [syserr.errcat.objects]: names "generic" and "system"; [syserr.errcat.virtuals]
    // default_error_condition(ev) of the generic category is error_condition(ev, *this).
    check(std::strcmp(std::generic_category().name(), "generic") == 0, "generic_category().name()");
    check(std::strcmp(std::system_category().name(), "system") == 0, "system_category().name()");
    check(std::strcmp(std::iostream_category().name(), "iostream") == 0, "iostream_category().name()");
    const std::error_code ec = std::make_error_code(std::errc::invalid_argument);
    check(ec == std::errc::invalid_argument && !ec.message().empty(), "error_code");
    check(std::error_code() == std::error_condition(), "error_code() == error_condition()");
    // [mem.res.global]/3: the default resource is initially new_delete_resource().
    check(std::pmr::get_default_resource() == std::pmr::new_delete_resource(), "get_default_resource()");
    std::pmr::vector<int> pv{1, 2, 3};
    check(pv.size() == 3 && pv.get_allocator().resource() == std::pmr::new_delete_resource(), "pmr::vector");
    // [re.alg.match]
    check(std::regex_match("abc123", std::regex("[a-z]+[0-9]+")), "regex_match");
    check(std::regex_search(std::string("x Y z"), std::regex("y", std::regex::icase)), "regex icase");
    // [rand.predef]/3: the 10000th invocation of a default-constructed mt19937.
    std::mt19937 g;
    g.discard(9999);
    check(g() == 4123659995u, "mt19937");
    // [except.throw], [string.access]: at() throws out_of_range.
    try {
      (void)std::string("abc").at(7);
      check(false, "at() did not throw");
    } catch (const std::out_of_range&) {
    }
    // [time.zone.db.access]/1: the first access initializes the database.
    const std::chrono::tzdb& db = std::chrono::get_tzdb();
    r.tz_version = db.version;
    const std::chrono::time_zone* utc = db.locate_zone("UTC");
    // (UTC may be a link to another zone, [time.zone.db.tzdb]/2.2; its offset is zero.)
    check(utc != nullptr && utc->get_info(std::chrono::sys_seconds{}).offset == std::chrono::seconds{0}, "locate_zone(\"UTC\")");
    check(std::chrono::locate_zone("UTC") == utc, "chrono::locate_zone");
    // [fs.op.current.path], [text.encoding.members]
    r.cwd = std::filesystem::current_path().native();
    r.env = std::text_encoding::environment();
    // [locale.statics]/1: "Causes future calls to the constructor locale() to return a copy of
    // the argument" (main checks that it sees this one).
    std::locale::global(std::locale(std::locale::classic(), new Comma));
    std::ostringstream os2;  // constructed with the new global locale ([ios.base.cons]: getloc() is locale())
    os2 << 2.5;
    check(os2.str() == "2,5", "stream after locale::global");
    r.constructed = true;
  }
};
FirstUser first_user;
}  // namespace

const StaticInitReport& static_init_report() { return first_user.r; }
std::locale static_init_global_locale_seen_there() { return std::locale(); }
char static_init_decimal_point_there() { return std::use_facet<std::numpunct<char>>(std::locale()).decimal_point(); }
