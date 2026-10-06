// Uses a layer this build does not have ('threads'): compiles, and fails to link with the missing
// primitive's name, ycxx_pal_sleep_until (examples/hosted-layers/README.md, "Absent layers").
#include <chrono>
#include <thread>

int main() { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
