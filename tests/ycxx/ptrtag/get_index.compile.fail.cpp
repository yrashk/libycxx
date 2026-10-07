// [ptrtag.pair.get]/1: get<I>(pointer_tag_pair): "Mandates: I < 2."
// EXPECT-ERROR: get
#include <memory>

int x;
auto v = std::get<2>(std::pointer_tag_pair<int*>(&x, 0u));
