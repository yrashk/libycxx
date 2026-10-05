// [rand.util.seedseq]/7: generate: "Mandates: iterator_traits<RandomAccessIterator>::value_type
// is an unsigned integer type capable of accommodating 32-bit quantities." long long is signed.
// COUNTERPART: libstdcxx:26_numerics/random/seed_seq/97311.cc
#include <random>

int main() {
  std::seed_seq q{1, 2, 3};
  long long out[4];
  q.generate(out, out + 4);
}
