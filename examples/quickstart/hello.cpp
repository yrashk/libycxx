import std;

int main() {
  std::vector<std::string> words{"modules", "import", "std"};
  std::ranges::sort(words);
  // prints ["import", "modules", "std"]
  std::println("{}", words);
}
