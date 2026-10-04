// [stoptoken.concepts]/12: "If an invocation of a callback exits via an exception then
// terminate shall be invoked ([except.terminate])."
#include <stop_token>
#include <exception>
#include <cstdlib>

int main() {
  std::set_terminate([] { std::_Exit(0); });
  std::stop_source s;
  std::stop_callback cb(s.get_token(), [] { throw 42; });
  s.request_stop();
  return 1;  // not reached
}
