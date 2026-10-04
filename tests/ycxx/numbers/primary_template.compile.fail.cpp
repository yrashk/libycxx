// [math.constants]/3: "A program that instantiates a primary template of a mathematical
// constant variable template is ill-formed." int is not a floating-point type, so pi_v<int>
// uses the primary template.
#include <numbers>

int x = std::numbers::pi_v<int>;
