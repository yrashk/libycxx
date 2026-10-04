// Exception-injection sweep over filesystem::path's modifiers, decomposition, iteration,
// generation and conversions: operator new fails at its k-th call, for every k until the
// operation completes.
//   [res.on.exception.handling]/1 and [fs.class.path]: no particular guarantee is stated for
//     these members, so the checks are the basic ones: no operator new block leaks, and a path
//     that was being modified is still a valid object (its native() string can be copied and
//     the path can be iterated and compared afterwards).
#include <filesystem>
#include <string>
#include "exc_new.hpp"

using namespace exh;
namespace fs = std::filesystem;

static const char long_a[] = "/a/rather/long/directory/name/that/does/not/fit/in/a/small/buffer";
static const char long_b[] = "another/fairly/long/relative/component/list/file.extension.tar.gz";

static void still_valid(const fs::path& p) {
  disarm();
  std::string s = p.native();
  long n = 0;
  for (auto& e : p) n += long(e.native().size());
  (void)(p == fs::path(s));
  (void)n;
}

template <class F>
void sw(const char* name, F op) {
  sweep_new(name, [&] {
    fs::path p(long_a);
    bool threw = attempt([&] { op(p); });
    still_valid(p);
    return threw;
  });
}

int main() {
  sw("path(const char*)", [](fs::path&) { fs::path q(long_b); });
  sw("path(string)", [](fs::path&) {
    std::string s(long_b);
    fs::path q(s);
  });
  sw("path(u8string)", [](fs::path&) { fs::path q(u8"/u/été/very/long/name/to/avoid/sso/file.txt"); });
  sw("path(wstring)", [](fs::path&) { fs::path q(L"/w/very/long/wide/name/to/avoid/sso/file.txt"); });
  sw("operator/=", [](fs::path& p) { p /= long_b; });
  sw("operator/= absolute", [](fs::path& p) { p /= "/root/absolute/long/enough/to/replace/the/whole/path"; });
  sw("operator+=", [](fs::path& p) { p += long_b; });
  sw("append(first, last)", [](fs::path& p) {
    std::string s(long_b);
    p.append(s.begin(), s.end());
  });
  sw("concat(first, last)", [](fs::path& p) {
    std::string s(long_b);
    p.concat(s.begin(), s.end());
  });
  sw("operator/(path, path)", [](fs::path& p) { fs::path q = p / long_b; });
  sw("replace_filename", [](fs::path& p) { p.replace_filename("replacement_file_name_long_enough.dat"); });
  sw("replace_extension", [](fs::path& p) { p.replace_extension(".a_much_longer_extension_than_usual"); });
  sw("remove_filename", [](fs::path& p) { p.remove_filename(); });
  sw("make_preferred", [](fs::path& p) { p.make_preferred(); });
  sw("decomposition", [](fs::path& p) {
    fs::path a = p.root_name(), b = p.root_directory(), c = p.root_path(), d = p.relative_path(),
             e = p.parent_path(), f = p.filename(), g = p.stem(), h = p.extension();
  });
  sw("iteration", [](fs::path& p) {
    std::size_t n = 0;
    for (auto& e : p) n += e.native().size();
    for (auto it = p.end(); it != p.begin();) n += (--it)->native().size();
  });
  sw("lexically_normal", [](fs::path&) { fs::path q = fs::path("/a/./b/../c//d/./../e/f/../../g/h").lexically_normal(); });
  sw("lexically_relative", [](fs::path& p) { fs::path q = p.lexically_relative("/a/rather/other/place/entirely"); });
  sw("lexically_proximate", [](fs::path& p) { fs::path q = p.lexically_proximate("x/y"); });
  sw("string conversions", [](fs::path& p) {
    std::string a = p.string();
    std::wstring b = p.wstring();
    std::u8string c = p.u8string();
    std::u16string d = p.u16string();
    std::u32string e = p.u32string();
    std::string f = p.generic_string();
    std::u8string g = p.generic_u8string();
  });
  sw("copy and swap", [](fs::path& p) {
    fs::path q(long_b);
    fs::path r(p);
    r = q;
    swap(p, q);
  });
  return finish();
}
