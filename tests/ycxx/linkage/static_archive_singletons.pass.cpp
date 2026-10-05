// A program whose code is split between its own translation unit and two members of a static
// archive: the library's program-wide entities are one entity for all of them.
//   [locale.facet]/6 and [locale.id]: a facet's static member id identifies the facet interface;
//     an inline static id is one object in every translation unit ([basic.def.odr]/15), so
//     has_facet/use_facet in one TU find a facet installed by another.
//   [locale.statics]/1-2: locale::global sets the global locale that default-constructed
//     locales copy everywhere in the program.
//   [ios.base.storage]/1: xalloc "Returns: index ++"; indices are unique in the program; /5
//     iword(idx) of the same object reads the same element in any TU.
//   [syserr.errcat.overview]/1: categories are compared by address; a function-local static
//     category object is one object.
//   [except.handle]/3: a handler for InlineError (vague-linkage type_info) matches the object
//     thrown in another TU; [expr.typeid]/1, [type.index.members]: typeid of the same type
//     compares equal across TUs; [any.nonmembers]/5: any_cast<Payload> succeeds on an any made
//     in another TU; [func.wrap.func.targ]: target<Callable>() is null for another type;
//     [util.smartptr.getdeleter]/1: get_deleter<Deleter> finds the deleter of a shared_ptr made
//     in another TU.
//   [mem.res.global]/4-6: set_default_resource changes what get_default_resource returns in the
//     whole program; [set.terminate]/[get.terminate]: one terminate handler;
//   [time.zone.db.access]/2: get_tzdb() returns a reference to the front of the one tzdb_list.
// ARCHIVE: ../support/linkage/archive_a.cpp ../support/linkage/archive_b.cpp
// REQUIRES: exceptions
#include "../support/linkage/archive_shared.hpp"
#include <chrono>
#include <cstdlib>
#include <sstream>
#include "check.hpp"

[[noreturn]] static void my_terminate() { std::abort(); }

int main() {
  std::locale tagged = make_tagged(std::locale::classic(), "from-archive");
  CHECK(std::has_facet<TagFacet>(tagged));
  CHECK(std::use_facet<TagFacet>(tagged).tag == "from-archive");
  CHECK(!std::has_facet<TagFacet>(std::locale::classic()));
  std::locale here(std::locale::classic(), new TagFacet("from-main"));
  CHECK(std::use_facet<TagFacet>(here).tag == "from-main");

  std::locale old = std::locale::global(here);
  CHECK(global_locale_tag_in_archive() == "from-main");
  set_global_tagged("set-in-archive");
  CHECK(std::use_facet<TagFacet>(std::locale()).tag == "set-in-archive");
  CHECK(global_locale_tag_in_archive() == "set-in-archive");
  std::locale::global(old);
  CHECK(global_locale_tag_in_archive() == "(none)");

  int i1 = std::ios_base::xalloc();
  int i2 = allocated_index();
  int i3 = std::ios_base::xalloc();
  CHECK(i1 != i2 && i2 != i3 && i1 != i3);
  std::stringstream s;
  s.iword(i2) = 1234;
  CHECK(read_iword(s, i2) == 1234);
  CHECK(read_iword(s, i1) == 0);

  CHECK(&archive_category() == &archive_category());
  std::error_code ec(3, archive_category());
  CHECK(ec.category() == archive_category());
  CHECK(ec.message() == "archive message 3");
  CHECK(ec != std::error_code(3, std::generic_category()));

  try {
    throw_inline_error(5);
    CHECK(false);
  } catch (const InlineError& e) {
    CHECK(e.code == 5 && std::string(e.what()) == "inline");
  }
  CHECK(payload_index() == std::type_index(typeid(Payload)));
  CHECK(payload_index() != std::type_index(typeid(Deleter)));
  std::any a = make_any_payload(8);
  CHECK(a.type() == typeid(Payload));
  CHECK(std::any_cast<Payload>(&a) != nullptr && std::any_cast<Payload>(a).v == 8);
  CHECK(std::any_cast<int>(&a) == nullptr);
  std::function<int()> f = make_function();
  CHECK(f() == 17);
  CHECK(f.target<int (*)()>() == nullptr);
  std::shared_ptr<int> sp = make_shared_with_deleter(4);
  CHECK(std::get_deleter<Deleter>(sp) != nullptr);
  CHECK(std::get_deleter<std::default_delete<int>>(sp) == nullptr);

  std::pmr::monotonic_buffer_resource mono;
  std::pmr::memory_resource* prev = std::pmr::set_default_resource(&mono);
  CHECK(default_resource_seen_in_archive() == &mono);
  set_default_resource_to(prev);
  CHECK(std::pmr::get_default_resource() == prev);

  std::terminate_handler th = std::set_terminate(my_terminate);
  CHECK(terminate_handler_seen_in_archive() == &my_terminate);
  std::set_terminate(th);

  CHECK(tzdb_address_in_archive() == &std::chrono::get_tzdb());
  return 0;
}
