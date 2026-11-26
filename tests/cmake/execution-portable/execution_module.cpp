import ycxx.execution.portable;

int main() {
  namespace ex = ycxx::execution;
  int value = 0;
  auto result = ex::sync_wait(ex::schedule(ex::get_parallel_scheduler()) | ex::then([&] { value = 42; }));
  ex::inplace_stop_source stop;
  stop.request_stop();
  bool stop_seen = false;
  auto env_result = ex::sync_wait(ex::write_env(
      ex::read_env(ex::get_stop_token) | ex::then([&](auto token) { stop_seen = token.stop_requested(); }),
      ex::prop(ex::get_stop_token, stop.get_token())));
  return result && value == 42 && env_result && stop_seen ? 0 : 1;
}
