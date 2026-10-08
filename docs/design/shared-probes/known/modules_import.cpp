// MODULES
// import std; with the inline namespace: the generated export lists (`export namespace std { using
// std::vector; ... }`) re-export members of std::__y1 (run-known.sh builds the module itself).
import std;
int main() {
  std::vector<int> v{1, 2, 3};
  std::string s = std::format("{}", v.size());
  std::println("{}", s == "3" ? "ok" : "FAIL");
  return s == "3" ? 0 : 1;
}
