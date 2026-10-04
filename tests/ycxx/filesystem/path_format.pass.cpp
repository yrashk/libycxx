// [fs.path.fmtr.funcs]: formatter<filesystem::path, charT> with path-format-spec
// "fill-and-align_opt width_opt ?_opt g_opt"; "If the ? option is used then the path is
// formatted as an escaped string"; /5 "Let s be p.generic_string<filesystem::path::value_type>()
// if the g option is used, otherwise p.native(). Writes s into ctx.out(), adjusted according to
// the path-format-spec."
#include <filesystem>
#include <format>
#include <string>
#include "check.hpp"

namespace fs = std::filesystem;

int main() {
  fs::path p("dir/file name");
  CHECK(std::format("{}", p) == "dir/file name");
  CHECK(std::format("{:g}", p) == "dir/file name");
  CHECK(std::format("{:?}", p) == "\"dir/file name\"");
  CHECK(std::format("{:?g}", p) == "\"dir/file name\"");
  CHECK(std::format("{:>16}", p) == "   dir/file name");
  CHECK(std::format("{:*<15}", p) == "dir/file name**");
  CHECK(std::format("{:^17?}", p) == " \"dir/file name\" ");
  CHECK(std::format("{:-^19?}", p) == "--\"dir/file name\"--");
  CHECK(std::format("{:?}", fs::path("tab\there")) == "\"tab\\there\"");
  CHECK(std::format("{}", fs::path()) == "");
  CHECK(std::format(L"{}", fs::path("w")) == L"w");
  return 0;
}
