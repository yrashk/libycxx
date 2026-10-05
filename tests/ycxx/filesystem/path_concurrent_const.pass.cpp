// [res.on.data.races]/3: const member functions of a shared filesystem::path ([fs.path.native.obs],
// [fs.path.generic.obs], [fs.path.compare], [fs.path.decompose], [fs.path.query],
// [fs.path.gen], [fs.path.itr]: iteration over a const path, which may compute its elements)
// and the non-member functions taking const path& ([fs.path.nonmember] hash_value, /, ==,
// <=>; [fs.path.io] operator<<; [fs.path.fmtr] formatting) do not modify it, so several
// threads may call them at once. Each thread also builds and modifies paths of its own. The
// results are compared with those computed before the threads started. Meant to be run under
// TSan.
// FLAGS: -pthread
#include <cstddef>
#include <filesystem>
#include <format>
#include <latch>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "check.hpp"

namespace fs = std::filesystem;
constexpr int K = 4;

struct Out {
  std::u8string generic;
  std::string native, filename, stem, ext, parent, root, rel, normal, relative, proximate;
  std::string elements, streamed, formatted, joined;
  std::wstring wide;
  std::u8string u8;
  std::size_t hash = 0;
  bool absolute = false, has_ext = false;
  int cmp = 0;
  bool operator==(const Out&) const = default;
};

static Out compute(const fs::path& p, const fs::path& base) {
  Out o;
  o.native = p.native();
  o.generic = p.generic_u8string();
  o.filename = p.filename().native();
  o.stem = p.stem().native();
  o.ext = p.extension().native();
  o.parent = p.parent_path().native();
  o.root = p.root_path().native();
  o.rel = p.relative_path().native();
  o.normal = p.lexically_normal().native();
  o.relative = p.lexically_relative(base).native();
  o.proximate = p.lexically_proximate(base).native();
  for (const fs::path& e : p) o.elements += "[" + e.native() + "]";
  for (auto it = p.end(); it != p.begin();) o.elements += "<" + (--it)->native() + ">";
  std::ostringstream os;
  os << p;
  o.streamed = os.str();
  o.formatted = std::format("{}|{:?}|{:g}", p, p, p);
  o.joined = (p / "sub" / ".." / "x.y").native();
  o.wide = p.wstring();
  o.u8 = p.u8string();
  o.hash = fs::hash_value(p);
  o.absolute = p.is_absolute();
  o.has_ext = p.has_extension();
  o.cmp = p.compare(base) < 0 ? -1 : p.compare(base) > 0 ? 1 : 0;
  return o;
}

int main() {
  const std::vector<fs::path> paths = {"/usr/local/lib/libx.so.1", "a/b/../c/./d.txt", "//net/share//dir/",
                                       "rel", "", "/", ".hidden", "dir/.", "x/y/z/"};
  const fs::path base = "/usr/share";
  std::vector<Out> expected;
  for (const auto& p : paths) expected.push_back(compute(p, base));
  CHECK(expected[0].filename == "libx.so.1" && expected[0].ext == ".1" && expected[0].relative == "../local/lib/libx.so.1");
  CHECK(expected[1].normal == "a/c/d.txt" && expected[1].elements.starts_with("[a][b][..][c][.][d.txt]"));
  std::latch go(K);
  std::vector<std::thread> ts;
  for (int k = 0; k < K; ++k)
    ts.emplace_back([&, k] {
      go.arrive_and_wait();
      for (int r = 0; r < 60; ++r) {
        const std::size_t i = static_cast<std::size_t>(k + r) % paths.size();
        CHECK(compute(paths[i], base) == expected[i]);
        CHECK(paths[i] == fs::path(paths[i]) && (paths[i] <=> paths[i]) == 0);
        fs::path own = paths[i];
        own /= "t" + std::to_string(k);
        own.replace_extension(".z");
        own.make_preferred();
        own.remove_filename();
        own += "q";
        CHECK(own.filename() == "q");
      }
    });
  for (auto& t : ts) t.join();
}
