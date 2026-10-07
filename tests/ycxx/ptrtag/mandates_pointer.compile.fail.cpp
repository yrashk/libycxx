// [ptrtag.pair.general]/4.1, /4.3: "Mandates: is_same_v<remove_cvref_t<Ptr>, Ptr> is true" and
// "is_pointer_v<Ptr> && !is_function_v<remove_pointer_t<Ptr>> is true."
// EXPECT-ERROR: Ptr must be an object pointer type
// EXPECT-ERROR: Ptr must be a cv-unqualified type
#include <memory>

std::pointer_tag_pair<void (*)(), 0> fp;
std::pointer_tag_pair<int* const, 2> cp;
