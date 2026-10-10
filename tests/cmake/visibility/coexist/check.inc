// Included by mine.cpp (built against libycxx, shared mode) and other.cpp (built against the
// toolchain's library, libstdc++ or libc++), each defining CHECK_NAME. Returns 31 when the library
// throws and catches its own exceptions (std::runtime_error, int), std::current_exception sees
// the first, an allocation failure goes through its own new_handler and its own bad_alloc, and
// std::string / std::vector work (heap-allocated strings, freed by the same library).
#include <cstddef>
#include <exception>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>
extern "C" int CHECK_NAME() {
  int r = 0;
  try {
    throw std::runtime_error("a message longer than any small-string buffer");
  } catch (const std::exception& e) {
    if (std::current_exception() && std::string(e.what()).size() == 45)
      r |= 1;
  }
  try {
    throw 42;
  } catch (int v) {
    if (v == 42)
      r |= 2;
  }
  static int handler_calls;
  handler_calls = 0;
  std::set_new_handler([] {
    ++handler_calls;
    std::set_new_handler(nullptr);
  });
  try {
    void* volatile p = ::operator new(static_cast<std::size_t>(-1) / 4);
    ::operator delete(p);
  } catch (const std::bad_alloc&) {
    if (handler_calls == 1)
      r |= 4;
  } catch (...) {
  }
  std::vector<std::string> v;
  for (int i = 0; i < 100; ++i)
    v.push_back(std::string(40, static_cast<char>('a' + i % 26)) + std::to_string(i));
  std::string joined;
  for (const auto& s : v)
    joined += s;
  if (joined.size() == 100 * 40 + 10 + 90 * 2 && joined.find("zzzz") != std::string::npos)
    r |= 8;
  try {
    (void)v.at(1000);
  } catch (const std::out_of_range&) {
    r |= 16;
  }
  return r;
}
