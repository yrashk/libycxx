// First member of the static archive of linkage/static_archive_singletons.pass.cpp.
#include "archive_shared.hpp"
#include <ios>

std::locale make_tagged(const std::locale& base, const char* tag) { return std::locale(base, new TagFacet(tag)); }

void set_global_tagged(const char* tag) { std::locale::global(make_tagged(std::locale::classic(), tag)); }

int allocated_index() { return std::ios_base::xalloc(); }

namespace {
struct ArchiveCategory : std::error_category {
  const char* name() const noexcept override { return "archive"; }
  std::string message(int c) const override { return "archive message " + std::to_string(c); }
};
}  // namespace

const std::error_category& archive_category() {
  static const ArchiveCategory c;
  return c;
}

void throw_inline_error(int c) { throw InlineError("inline", c); }

std::any make_any_payload(int v) { return Payload{v}; }

namespace {
struct Callable {
  int operator()() const { return 17; }
};
}  // namespace
std::function<int()> make_function() { return std::function<int()>(Callable{}); }

std::type_index payload_index() { return typeid(Payload); }
