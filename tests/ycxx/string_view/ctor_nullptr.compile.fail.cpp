// [string.view.template.general]: "basic_string_view(nullptr_t) = delete;" (P2166).
#include <string_view>

std::string_view s = nullptr;
