// [inout.ptr.t]/3: "If Smart is a specialization of shared_ptr, the program is ill-formed."
#include <memory>

int c_replace(int** pp);

int main() {
  std::shared_ptr<int> s;
  c_replace(std::inout_ptr(s));
}
