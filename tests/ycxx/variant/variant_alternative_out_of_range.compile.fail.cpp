// [variant.helper]/4: variant_alternative<I, variant<Types...>>::type:
// "Mandates: I < sizeof...(Types)."
#include <variant>

using T = std::variant_alternative_t<2, std::variant<int, long>>;
T* p = nullptr;
