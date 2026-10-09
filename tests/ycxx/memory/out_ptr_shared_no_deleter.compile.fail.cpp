// EXPECT-ERROR: error: static assertion failed[^\n]*std::out_ptr: a shared_ptr needs a deleter argument
// [out.ptr.t]/3: "If Smart is a specialization of shared_ptr and sizeof...(Args) == 0, the
// program is ill-formed."
#include <memory>

int c_create(int** pp);

int main() {
  std::shared_ptr<int> s;
  c_create(std::out_ptr(s));
}
