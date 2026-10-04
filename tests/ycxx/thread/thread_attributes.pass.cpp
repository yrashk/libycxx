// [thread.attributes]: thread::name_hint<char> and thread::stack_size_hint may precede the
// callable in the thread / jthread constructor ([thread.thread.constr]/4: F is the first
// argument whose decayed type is not a thread attribute type); "If size is zero, the thread
// attribute is ignored." name_hint is not copyable or movable; jthread::name_hint and
// jthread::stack_size_hint name the same types.
// FLAGS: -pthread
#include <thread>
#include <string_view>
#include <type_traits>
#include "check.hpp"

static_assert(!std::is_copy_constructible_v<std::thread::name_hint<char>>);
static_assert(!std::is_move_constructible_v<std::thread::name_hint<char>>);
static_assert(!std::is_convertible_v<std::string_view, std::thread::name_hint<char>>);
static_assert(!std::is_convertible_v<std::size_t, std::thread::stack_size_hint>);
static_assert(std::is_nothrow_constructible_v<std::thread::stack_size_hint, std::size_t>);
static_assert(std::is_same_v<std::jthread::stack_size_hint, std::thread::stack_size_hint>);
static_assert(std::is_same_v<std::jthread::name_hint<char>, std::thread::name_hint<char>>);

constexpr std::thread::stack_size_hint constant_hint(1 << 20);

int main() {
  int r = 0;
  std::thread t(std::thread::name_hint<char>("worker"), [&](int v) { r = v; }, 3);
  t.join();
  CHECK(r == 3);
  std::thread t2(std::thread::stack_size_hint(1 << 20), std::thread::name_hint<char>("w2"),
                 [&] { r = 4; });
  t2.join();
  CHECK(r == 4);
  std::thread t3(std::thread::stack_size_hint(0), [&] { r = 5; });
  t3.join();
  CHECK(r == 5);
  std::jthread j(std::jthread::name_hint<char>("j"), [&](std::stop_token st) { r = st.stop_possible() ? 6 : -1; });
  j.join();
  CHECK(r == 6);
  return 0;
}
