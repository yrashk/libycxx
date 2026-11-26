import ycxx.execution.portable;

int main() {
  namespace ex = ycxx::execution;
  int value = 0;
  auto result = ex::sync_wait(ex::schedule(ex::get_parallel_scheduler()) | ex::then([&] { value = 42; }));
  return result && value == 42 ? 0 : 1;
}
