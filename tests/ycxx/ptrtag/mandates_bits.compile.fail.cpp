// [ptrtag.pair.general]/4.6: "Mandates: BitsRequested <= max_pointer_bits_available is true."
// EXPECT-ERROR: BitsRequested exceeds max_pointer_bits_available
#include <memory>

std::pointer_tag_pair<int*, std::max_pointer_bits_available + 1> p;
