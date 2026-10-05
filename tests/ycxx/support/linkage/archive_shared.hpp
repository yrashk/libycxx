// Declarations shared by linkage/static_archive_singletons.pass.cpp and the members of its
// static archive (archive_a.cpp, archive_b.cpp).
#pragma once
#include <any>
#include <exception>
#include <functional>
#include <locale>
#include <memory>
#include <memory_resource>
#include <stdexcept>
#include <string>
#include <system_error>
#include <typeindex>

// A facet with an inline static id: one entity in every translation unit ([basic.def.odr]/15).
struct TagFacet : std::locale::facet {
  static inline std::locale::id id;
  std::string tag;
  explicit TagFacet(std::string t, std::size_t refs = 0) : std::locale::facet(refs), tag(std::move(t)) {}
};

// A polymorphic type with only inline members (vague linkage: its type_info may be emitted in
// every translation unit that uses it).
struct InlineError : std::runtime_error {
  int code;
  InlineError(const char* w, int c) : std::runtime_error(w), code(c) {}
};

struct Payload {
  int v;
};
struct Deleter {
  void operator()(int* p) const { delete p; }
};

// archive_a.cpp
std::locale make_tagged(const std::locale& base, const char* tag);
void set_global_tagged(const char* tag);
int allocated_index();
const std::error_category& archive_category();
void throw_inline_error(int c);
std::any make_any_payload(int v);
std::function<int()> make_function();
std::type_index payload_index();

// archive_b.cpp
std::shared_ptr<int> make_shared_with_deleter(int v);
void set_default_resource_to(std::pmr::memory_resource* r);
std::pmr::memory_resource* default_resource_seen_in_archive();
std::terminate_handler terminate_handler_seen_in_archive();
const void* tzdb_address_in_archive();
std::string global_locale_tag_in_archive();
int read_iword(std::ios_base& s, int index);
