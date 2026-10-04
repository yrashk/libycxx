// [time.duration.general]/2: a cv-qualified Rep makes the program ill-formed.
#include <chrono>

std::chrono::duration<const int> d(5);

int main() {}
