// EXPECT-ERROR: error: static assertion failed[^\n]*uniform_int_distribution: IntType must be a standard integer type \(\[rand\.req\.genl\]/1\.6\)
// [rand.req.genl]/1.6: "If a template argument corresponding to a template parameter named IntType
// is neither a standard signed nor a standard unsigned integer type, nor an extended integer type
// [of suitable width], nor a member of an implementation-defined subset of integer types, the
// program is ill-formed." char is neither a signed nor an unsigned integer type.
#include <random>

int main() {
  std::mt19937 g;
  std::uniform_int_distribution<char> d('a', 'z');
  return d(g);
}
