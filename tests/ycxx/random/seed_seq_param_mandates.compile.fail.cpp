// [rand.util.seedseq]/13: param(dest): "Mandates: Values of type result_type are writable to dest."
#include <random>

struct sink {};

int main() {
  std::seed_seq q{1, 2, 3};
  sink* out[3];
  q.param(out);
}
