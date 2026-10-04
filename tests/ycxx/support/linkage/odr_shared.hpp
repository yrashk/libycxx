// Shared header of linkage/odr_inline_entities.pass.cpp and support/linkage/odr_tu2.cpp.
#pragma once
#include <atomic>
#include <format>
#include <ios>
#include <locale>
#include <system_error>
#include <map>
#include <string>
#include <typeindex>
#include <vector>

// [dcl.inline]/7: "An inline function or variable with external linkage shall have the same
// address in all translation units." [basic.def.odr]/15-16: one entity.
inline std::string shared_greeting = "hello";
inline std::vector<int> shared_numbers{1, 2, 3};
inline std::atomic<int> shared_counter{0};
inline thread_local std::string shared_tl = "tl";

// A block-scope static of an inline function is one object in every TU ([dcl.inline]/7).
inline std::map<std::string, int>& registry() {
  static std::map<std::string, int> m;
  return m;
}

struct Point {
  int x, y;
};
// One program-defined specialization of std::formatter, used from both TUs.
template <>
struct std::formatter<Point> {
  constexpr auto parse(std::format_parse_context& pc) { return pc.begin(); }
  template <class FormatContext>  // generic: [format.formattable] uses an unspecified iterator
  auto format(const Point& p, FormatContext& fc) const {
    return std::format_to(fc.out(), "({}, {})", p.x, p.y);
  }
};

// A program-defined facet ([locale.facet]): its static id member identifies it in every TU
// ([locale.id]: "The class locale::id provides identification of a locale facet interface,
// used as an index for lookup and to encapsulate initialization").
struct Greeter : std::locale::facet {
  static inline std::locale::id id;
  std::string word;
  explicit Greeter(std::string w) : word(std::move(w)) {}
};

// Implemented in odr_tu2.cpp.
bool tu2_has_greeter(const std::locale& loc);
const Greeter* tu2_use_greeter(const std::locale& loc);
std::locale tu2_make_greeter_locale();
int tu2_xalloc_index();
long& tu2_iword(std::ios_base& s);
const std::string* tu2_greeting();
const std::vector<int>* tu2_numbers();
std::atomic<int>* tu2_counter();
const std::string* tu2_tl();
std::map<std::string, int>* tu2_registry();
std::string tu2_format(Point p);
std::type_index tu2_type_index();
const std::type_info& tu2_type_info();
std::size_t tu2_hash(const std::string& s);
std::string tu2_locale_name();
std::ios_base::fmtflags tu2_cout_flags();
std::string tu2_error_message(int ev);
const std::error_category* tu2_generic_category();
const std::error_category* tu2_system_category();
const void* tu2_cout_address();
