import ycxx.execution.portable;

int main() {
  namespace ex = ycxx::execution;
  int value = 0;
  auto result = ex::sync_wait(ex::schedule(ex::get_parallel_scheduler()) | ex::then([&] { value = 42; }));
  int items[3] = {};
  auto bulk_result = ex::sync_wait(ex::bulk(ex::schedule(ex::get_parallel_scheduler()), ex::par, 3,
      [&](int index) noexcept { items[index] = index + 1; }));
  ex::inplace_stop_source stop;
  stop.request_stop();
  bool stop_seen = false;
  auto env_result = ex::sync_wait(ex::write_env(
      ex::read_env(ex::get_stop_token) | ex::then([&](auto token) { stop_seen = token.stop_requested(); }),
      ex::prop(ex::get_stop_token, stop.get_token())));
  return result && value == 42 && bulk_result && items[0] == 1 && items[1] == 2 && items[2] == 3 &&
      env_result && stop_seen ? 0 : 1;
}
