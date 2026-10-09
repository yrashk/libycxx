// EXPECT-ERROR: error: static assertion failed[^\n]*duration: Rep must not be a duration \(\[time\.duration\.general\]/2\)
// [time.duration.general]/2: "If a specialization of duration is instantiated with a cv-qualified
// type or a specialization of duration as the argument for the template parameter Rep, the program
// is ill-formed."
#include <chrono>

std::chrono::duration<std::chrono::seconds> d(std::chrono::seconds(1));

int main() {}
