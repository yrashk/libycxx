#include <ycxx/execution/stop_token.hpp>

[[gnu::visibility("default")]]
bool request_stop_from_shared_library(ycxx::execution::inplace_stop_source& source) noexcept {
  return source.request_stop();
}
