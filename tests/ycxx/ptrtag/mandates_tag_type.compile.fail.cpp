// [ptrtag.pair.general]/4.2, /4.5: "Mandates: is_same_v<remove_cvref_t<TagT>, TagT> is true" and
// "sizeof(TagT) <= sizeof(void*) is true."
// EXPECT-ERROR: TagT must be a cv-unqualified type
// EXPECT-ERROR: TagT must not be larger than a pointer
#include <memory>

std::pointer_tag_pair<int*, 2, const unsigned> c;
std::pointer_tag_pair<int*, 2, unsigned __int128> big;
