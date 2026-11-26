#include <ycxx/execution/stop_token.hpp>
#include <chrono>
#include <future>
#include <latch>
#include <memory>
#include <thread>
#include "check.hpp"

namespace ex = ycxx::execution;
bool request_stop_from_shared_library(ex::inplace_stop_source& source) noexcept;

int main() {
  ex::inplace_stop_source source;
  std::latch entered(1);
  std::latch release(1);
  auto run = [&] {
    entered.count_down();
    release.wait();
  };
  auto callback = std::make_unique<ex::inplace_stop_callback<decltype(run)>>(source.get_token(), run);
  std::thread requester([&] { CHECK(request_stop_from_shared_library(source)); });
  entered.wait();

  std::latch deregistering(1);
  std::promise<void> deregistered;
  auto finished = deregistered.get_future();
  std::thread remover([&] {
    deregistering.count_down();
    callback.reset();
    deregistered.set_value();
  });
  deregistering.wait();
  CHECK(finished.wait_for(std::chrono::milliseconds(150)) == std::future_status::timeout);
  release.count_down();
  remover.join();
  requester.join();
  CHECK(finished.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
}
