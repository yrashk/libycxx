// Second translation unit of linkage/odr_inline_entities.pass.cpp.
#include <functional>
#include <iostream>
#include <locale>
#include <system_error>
#include "odr_shared.hpp"

namespace {
// Dynamic initialisation of a static object of this TU registers into the inline function's
// block-scope static (initialised on first use, so the cross-TU order does not matter).
struct Registrar {
  Registrar() {
    registry()["tu2"] = 2;
    ++shared_counter;
  }
} registrar;
// ios_base::xalloc during dynamic initialisation of this TU.
const int tu2_index = std::ios_base::xalloc();
}  // namespace

const std::string* tu2_greeting() { return &shared_greeting; }
const std::vector<int>* tu2_numbers() { return &shared_numbers; }
std::atomic<int>* tu2_counter() { return &shared_counter; }
const std::string* tu2_tl() { return &shared_tl; }
std::map<std::string, int>* tu2_registry() { return &registry(); }
std::string tu2_format(Point p) { return std::format("{:}|{}", p, std::vector<Point>{p}); }
std::type_index tu2_type_index() { return typeid(std::map<std::string, std::vector<int>>); }
const std::type_info& tu2_type_info() { return typeid(std::vector<Point>); }
std::size_t tu2_hash(const std::string& s) { return std::hash<std::string>{}(s); }
std::string tu2_locale_name() { return std::locale().name(); }
std::ios_base::fmtflags tu2_cout_flags() { return std::cout.flags(); }
std::string tu2_error_message(int ev) { return std::generic_category().message(ev); }
const std::error_category* tu2_generic_category() { return &std::generic_category(); }
const std::error_category* tu2_system_category() { return &std::system_category(); }
const void* tu2_cout_address() { return &std::cout; }
bool tu2_has_greeter(const std::locale& loc) { return std::has_facet<Greeter>(loc); }
const Greeter* tu2_use_greeter(const std::locale& loc) { return &std::use_facet<Greeter>(loc); }
std::locale tu2_make_greeter_locale() { return std::locale(std::locale::classic(), new Greeter("tu2")); }
int tu2_xalloc_index() { return tu2_index; }
long& tu2_iword(std::ios_base& s) { return s.iword(tu2_index); }
