// Uses a layer this build does not have ('threads'): fails to compile, with a static_assert that
// names the layer (examples/hosted-layers/README.md, "Absent layers").
#include <thread>

int main() {
  std::thread t([] {});
  t.join();
}
