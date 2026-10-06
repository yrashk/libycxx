// A small program using libycxx through the standard library modules ([std.modules]): no
// #include at all. Prints "libycxx example: ok" and returns 0 when every check holds.
import std.compat; // std, and the C library's names in the global namespace

int main() {
  std::vector<std::string> words{"modules", "import", "std"};
  std::ranges::sort(words);
  auto joined = std::format("{}", words);
  std::map<std::string, std::size_t> lengths;
  for (const auto& w : words)
    lengths[w] = ::strlen(w.c_str());
  bool ok = joined == R"(["import", "modules", "std"])" && lengths.at("modules") == 7;
  try {
    (void)words.at(9);
    ok = false;
  } catch (const std::out_of_range&) {
  }
  if (!ok)
    return 1;
  std::println("libycxx example: ok");
  return 0;
}
