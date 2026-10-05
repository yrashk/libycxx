// Second member of the static archive of linkage/static_archive_singletons.pass.cpp.
#include "archive_shared.hpp"
#include <chrono>
#include <ios>

std::shared_ptr<int> make_shared_with_deleter(int v) { return std::shared_ptr<int>(new int(v), Deleter{}); }

void set_default_resource_to(std::pmr::memory_resource* r) { std::pmr::set_default_resource(r); }

std::pmr::memory_resource* default_resource_seen_in_archive() { return std::pmr::get_default_resource(); }

std::terminate_handler terminate_handler_seen_in_archive() { return std::get_terminate(); }

const void* tzdb_address_in_archive() { return &std::chrono::get_tzdb(); }

std::string global_locale_tag_in_archive() {
  std::locale l;
  return std::has_facet<TagFacet>(l) ? std::use_facet<TagFacet>(l).tag : "(none)";
}

int read_iword(std::ios_base& s, int index) { return static_cast<int>(s.iword(index)); }
