// Uses a layer this build does not have ('random'): compiles, and fails to link with the missing
// primitive's name, ycxx_pal_random_open (examples/hosted-layers/README.md, "Absent layers").
#include <random>

int main() {
  std::random_device rd;
  return static_cast<int>(rd() & 1);
}
