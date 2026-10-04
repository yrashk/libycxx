// [rand.util.seedseq]/4: seed_seq(InputIterator begin, InputIterator end): "Mandates:
// iterator_traits<InputIterator>::value_type is an integer type."
#include <random>

int main() {
  const double in[2] = {1.0, 2.0};
  std::seed_seq q(in, in + 2);
  return int(q.size());
}
