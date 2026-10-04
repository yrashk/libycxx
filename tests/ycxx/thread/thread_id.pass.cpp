// [thread.thread.id]: a default id represents no thread; ids of distinct threads differ; /3
// "thread::id is a trivially copyable class"; operator== and operator<=> (strong_ordering,
// a total order); /9 operator<< writes a text representation equal for equal ids and distinct
// for distinct ids; /11-13 formatter<thread::id> (fill-and-align, width; default alignment
// '>'); /14 hash<thread::id> is enabled. [thread.thread.this]: this_thread::get_id().
// FLAGS: -pthread
#include <thread>
#include <compare>
#include <format>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_trivially_copyable_v<std::thread::id>);
static_assert(std::is_nothrow_default_constructible_v<std::thread::id>);
static_assert(std::is_same_v<decltype(std::thread::id() <=> std::thread::id()), std::strong_ordering>);
static_assert(noexcept(std::thread::id() == std::thread::id()));
static_assert(noexcept(std::this_thread::get_id()));
static_assert(std::is_same_v<std::jthread::id, std::thread::id>);

static std::string str(std::thread::id id) {
  std::ostringstream os;
  os << id;
  return os.str();
}

int main() {
  std::thread::id none, none2;
  CHECK(none == none2);
  CHECK((none <=> none2) == 0);
  std::thread::id me = std::this_thread::get_id();
  CHECK(me != none);
  CHECK(me == std::this_thread::get_id());

  std::thread::id other;
  std::thread t([&] { other = std::this_thread::get_id(); });
  t.join();
  CHECK(other != me && other != none);

  // total order
  CHECK(((me <=> other) < 0) != ((other <=> me) < 0));
  CHECK((me < other) != (me > other));
  std::set<std::thread::id> s{me, other, none, me};
  CHECK(s.size() == 3);

  // text representation
  CHECK(str(me) == str(std::this_thread::get_id()));
  CHECK(str(me) != str(other));
  CHECK(str(me) != str(none));
  CHECK(!str(me).empty());
  std::wostringstream ws;
  ws << me;
  CHECK(!ws.str().empty());

  // formatter: default right alignment
  std::string f = std::format("{}", me);
  CHECK(f == str(me));
  std::size_t w3 = f.size() + 3, w2 = f.size() + 2, w1 = f.size() + 1;
  std::string w = std::vformat("{:*>{}}", std::make_format_args(me, w3));
  CHECK(w == "***" + f);
  std::string l = std::vformat("{:_<{}}", std::make_format_args(me, w2));
  CHECK(l == f + "__");
  std::string d = std::vformat("{:{}}", std::make_format_args(me, w1));
  CHECK(d == " " + f);  // the default alignment is '>'
  std::string c = std::vformat("{:^{}}", std::make_format_args(me, w2));
  CHECK(c == " " + f + " ");
  CHECK(std::format(L"{}", me).size() == f.size());

  std::hash<std::thread::id> h;
  CHECK(h(me) == h(std::this_thread::get_id()));
  static_assert(std::is_same_v<decltype(h(me)), std::size_t>);
  return 0;
}
