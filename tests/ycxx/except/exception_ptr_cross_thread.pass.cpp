// [propagation]: an exception_ptr may refer to an exception caught in another thread; "The
// referenced object shall remain valid at least as long as there is an exception_ptr object
// that refers to it." and rethrow_exception in a different thread throws it (or a copy) there.
// [except.throw]/4 (Note 3): "A thrown exception does not propagate to other threads unless
// caught, stored, and rethrown using appropriate library functions".
// FLAGS: -pthread
// REQUIRES: exceptions
#include <exception>
#include <stdexcept>
#include <pthread.h>
#include <cstring>
#include "check.hpp"

static std::exception_ptr stored;
static int uncaught_in_thread = -1;

static void* worker(void*) {
  try {
    throw std::runtime_error("from thread");
  } catch (...) {
    stored = std::current_exception();
  }
  uncaught_in_thread = std::uncaught_exceptions();
  return nullptr;
}

int main() {
  pthread_t t;
  CHECK(pthread_create(&t, nullptr, worker, nullptr) == 0);
  CHECK(pthread_join(t, nullptr) == 0);
  CHECK(uncaught_in_thread == 0);
  CHECK(stored != nullptr);
  CHECK(std::current_exception() == nullptr);  // per-thread: nothing handled here
  bool caught = false;
  try {
    std::rethrow_exception(stored);
  } catch (const std::runtime_error& e) {
    caught = std::strcmp(e.what(), "from thread") == 0;
    CHECK(std::current_exception() != nullptr);
  }
  CHECK(caught);
  stored = nullptr;
  return 0;
}
