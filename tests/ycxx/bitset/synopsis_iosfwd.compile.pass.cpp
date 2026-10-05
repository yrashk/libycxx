// [bitset.syn]: <bitset> includes <string> and <iosfwd> (for istream, ostream), so their names
// are available after including <bitset> alone.
#include <bitset>

std::ios* a;
std::istream* b;
std::ostream* c;
std::iostream* d;
std::string* e;

int main() {}
