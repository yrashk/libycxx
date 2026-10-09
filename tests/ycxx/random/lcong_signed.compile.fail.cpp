// EXPECT-ERROR: error: static assertion failed[^\n]*linear_congruential_engine: UIntType must be an unsigned integer type at least as wide as short \(\[rand\.req\.genl\]/1\.7\)
// [rand.req.genl]/1.7: "If a template argument corresponding to a template parameter named UIntType
// is neither a standard or extended unsigned integer type whose width is greater or equal to that
// of short and less than or equal to that of long long, nor a member of an implementation-defined
// subset of unsigned integer types, the program is ill-formed." int is signed.
#include <random>

int main() {
  std::linear_congruential_engine<int, 16807, 0, 2147483647> e;
  return int(e());
}
