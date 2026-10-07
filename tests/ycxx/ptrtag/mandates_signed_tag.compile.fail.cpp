// [ptrtag.pair.general]/4.4: "Mandates: is_unsigned_v<UT> is true, where UT is
// underlying_type_t<TagT> if TagT is an enumeration type, and TagT otherwise."
// EXPECT-ERROR: TagT must be an unsigned integer type
#include <memory>

enum class Signed : int { a };
std::pointer_tag_pair<int*, 2, Signed> p;
