// EXPECT-ERROR: error: static assertion failed[^\n]*seed_seq::generate: the iterator's value_type must be an unsigned integer type of at least 32 bits
// [rand.util.seedseq]/7: generate: "Mandates: iterator_traits<RandomAccessIterator>::value_type
// is an unsigned integer type capable of accommodating 32-bit quantities."
#include <random>
#include <cstdint>

int main() {
  std::seed_seq q{1, 2, 3};
  std::uint16_t out[4];
  q.generate(out, out + 4);
}
