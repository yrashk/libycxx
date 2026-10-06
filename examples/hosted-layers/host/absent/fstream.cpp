// Uses a layer this build does not have (the C library, 'clib', which iostreams and the file
// streams need): stops at the #include with an #error that names the layer
// (examples/hosted-layers/README.md, "Absent layers").
#include <fstream>

int main() {
  std::ofstream out("hosted-layers.txt");
  out << "never built\n";
}
