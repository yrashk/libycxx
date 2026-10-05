// The time zone database. [time.zone.db.tzdb]/1: "Each vector in a tzdb object is sorted";
// /2-3: tzdb::locate_zone finds a zone by name, else a link by name and then the zone named by
// its target(), else throws runtime_error. [time.zone.db.list]/1-13: tzdb_list is a
// non-copyable singleton whose const_iterator is a Cpp17ForwardIterator with value type tzdb;
// front() is the first tzdb. [time.zone.db.access]/1-8: get_tzdb() is
// get_tzdb_list().front(); the namespace-scope locate_zone / current_zone forward to it.
// [time.zone.overview], [time.zone.link.overview]: time_zone and time_zone_link are movable,
// not copyable; [time.zone.nonmembers], [time.zone.link.nonmembers]: == and <=> compare
// name(). [time.zone.leap.overview]: leap_second is copyable; [time.zone.leap.nonmembers]:
// comparisons by date(). [time.zone.zonedtraits]/2-3: zoned_traits<const time_zone*>.
// (Uses zones and links of the IANA database that have existed for decades: America/New_York,
// Europe/London, Asia/Kolkata and the link US/Eastern -> America/New_York.)
// COUNTERPART: libcxx:time/time.zone/time.zone.link/time.zone.link.members/(name|target).pass.cpp
// COUNTERPART: libcxx:time/time.zone/time.zone.timezone/time.zone.members/name.pass.cpp
#include <chrono>
#include <algorithm>
#include <compare>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

static_assert(!std::is_copy_constructible_v<tzdb_list> && !std::is_copy_assignable_v<tzdb_list>);
static_assert(!std::is_copy_constructible_v<time_zone> && std::is_move_constructible_v<time_zone>);
static_assert(!std::is_copy_constructible_v<time_zone_link> && std::is_move_constructible_v<time_zone_link>);
static_assert(std::is_copy_constructible_v<leap_second> && std::is_copy_assignable_v<leap_second>);
static_assert(std::forward_iterator<tzdb_list::const_iterator>);
static_assert(std::is_same_v<std::iter_value_t<tzdb_list::const_iterator>, tzdb>);
static_assert(std::is_same_v<std::iter_reference_t<tzdb_list::const_iterator>, const tzdb&>);
static_assert(std::is_same_v<decltype(get_tzdb()), const tzdb&>);
static_assert(std::is_same_v<decltype(locate_zone("UTC")), const time_zone*>);
static_assert(std::is_same_v<decltype(std::declval<const time_zone&>().name()), std::string_view>);
static_assert(noexcept(std::declval<const time_zone&>().name()));
static_assert(noexcept(std::declval<const time_zone_link&>().target()));
static_assert(noexcept(std::declval<const tzdb_list&>().front()) && noexcept(std::declval<const tzdb_list&>().begin()));
static_assert(std::is_same_v<decltype(std::declval<const time_zone&>() <=> std::declval<const time_zone&>()),
                             std::strong_ordering>);

int main() {
  tzdb_list& list = get_tzdb_list();
  CHECK(&list == &get_tzdb_list());  // a singleton
  CHECK(std::distance(list.begin(), list.end()) >= 1 && list.cbegin() == list.begin() && list.cend() == list.end());
  const tzdb& db = get_tzdb();
  CHECK(&db == &list.front() && &*list.begin() == &db);
  CHECK(!db.version.empty());
  CHECK(!db.zones.empty() && !db.links.empty() && !db.leap_seconds.empty());

  // Every vector is sorted.
  CHECK(std::ranges::is_sorted(db.zones));
  CHECK(std::ranges::is_sorted(db.links));
  CHECK(std::ranges::is_sorted(db.leap_seconds));
  CHECK(std::ranges::adjacent_find(db.zones, [](auto& a, auto& b) { return a.name() == b.name(); }) == db.zones.end());

  // Zones by name.
  const time_zone* ny = db.locate_zone("America/New_York");
  CHECK(ny->name() == "America/New_York");
  CHECK(locate_zone("America/New_York") == ny);
  CHECK(std::ranges::find(db.zones, std::string_view("America/New_York"), &time_zone::name) != db.zones.end());
  CHECK(&*std::ranges::find(db.zones, std::string_view("America/New_York"), &time_zone::name) == ny);
  const time_zone* london = locate_zone("Europe/London");
  CHECK(london->name() == "Europe/London" && london != ny);
  CHECK(*ny == *ny && !(*ny == *london) && (*ny <=> *london) == std::strong_ordering::less);
  CHECK((*ny <=> *locate_zone("Asia/Kolkata")) == std::strong_ordering::less);  // "America" < "Asia"

  // Links: the zone named by the link's target.
  auto link = std::ranges::find(db.links, std::string_view("US/Eastern"), &time_zone_link::name);
  if (link != db.links.end()) {  // the link lives in the "backward" file, present in every common build
    CHECK(link->target() == "America/New_York");
    CHECK(locate_zone("US/Eastern") == ny);
  }
  for (const time_zone_link& l : db.links) {
    CHECK(l == l && !(l < l));
    if (std::ranges::find(db.zones, l.name(), &time_zone::name) == db.zones.end()) {
      auto target = std::ranges::find(db.zones, l.target(), &time_zone::name);
      if (target != db.zones.end()) CHECK(db.locate_zone(l.name()) == &*target);
    }
  }
  // "UTC" names a zone or a link; either way some zone is found ([time.zone.zonedtraits]/2).
  const time_zone* utc = locate_zone("UTC");
  CHECK(utc != nullptr && zoned_traits<const time_zone*>::default_zone() == utc);
  CHECK(zoned_traits<const time_zone*>::locate_zone("America/New_York") == ny);

  // Unknown names.
  for (std::string_view bad : {"Not/A_Zone", "america/new_york", "", "America/New_York "}) {
    bool thrown = false;
    try {
      (void)locate_zone(bad);
    } catch (const std::runtime_error&) {
      thrown = true;
    }
    CHECK(thrown);
  }

  // current_zone: some zone of the database.
  const time_zone* cur = current_zone();
  CHECK(cur != nullptr && cur == db.current_zone());
  CHECK(locate_zone(cur->name())->name() == cur->name());

  // leap_second comparisons ([time.zone.leap.nonmembers]).
  const leap_second& first = db.leap_seconds.front();
  const leap_second copy = first;
  CHECK(copy == first && copy.date() == first.date() && copy.value() == first.value());
  CHECK(first == first.date() && first < first.date() + 1ns && first.date() - 1s < first);
  CHECK(first > first.date() - 1ms && first.date() + 1s > first);
  CHECK(first <= first.date() && first.date() <= first && first >= first.date() && first.date() >= first);
  CHECK((first <=> first.date()) == 0 && (first <=> first.date() + 1min) < 0);
  CHECK((db.leap_seconds[0] <=> db.leap_seconds[1]) == std::strong_ordering::less);
  return 0;
}
