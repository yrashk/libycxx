// The host: dlopen()s the plugin (argv[1]), exchanges std::string/std::vector both ways and catches
// the plugin's exception as std::runtime_error. Prints "plugin 15 shared-state N": N has a bit for
// each piece of runtime state the plugin sees as the host does (15: one runtime per process;
// expected only when both use libycxx.so).
#include "api.hpp"
#include <cstdio>
#include <dlfcn.h>
#include <exception>
#include <stdexcept>
#include <cstdlib>
int main(int argc, char** argv) {
  if (argc < 2) return 2;
  void* h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!h) { std::printf("dlopen: %s\n", dlerror()); return 2; }
  auto make = reinterpret_cast<plugin_make_t>(dlsym(h, "plugin_make"));
  auto consume = reinterpret_cast<plugin_consume_t>(dlsym(h, "plugin_consume"));
  auto thrower = reinterpret_cast<plugin_throw_t>(dlsym(h, "plugin_throw"));
  auto uncaught = reinterpret_cast<plugin_uncaught_t>(dlsym(h, "plugin_uncaught"));
  if (!make || !consume || !thrower || !uncaught) { std::printf("dlsym: missing symbol\n"); return 2; }
  int r = 0;
  // Made by the plugin, read, grown and freed by the host.
  std::vector<std::string>* v = make(std::string("seed-longer-than-the-small-buffer-"), 10);
  v->push_back("host");
  if (v->size() == 11 && (*v)[3].ends_with("3") && (*v)[10] == "host")
    r |= 1;
  delete v;
  // Made by the host, freed by the plugin.
  auto* w = new std::vector<std::string>(5, std::string(50, 'h'));
  if (consume(w) == 250)
    r |= 2;
  // The plugin's exception, caught by type and by its base.
  try {
    thrower("from the plugin, longer than the small buffer");
  } catch (const std::runtime_error& e) {
    if (std::string(e.what()) == "from the plugin, longer than the small buffer")
      r |= 4;
  } catch (...) {
  }
  try {
    thrower("again");
  } catch (const std::exception& e) {
    if (std::string(e.what()) == "again" && std::uncaught_exceptions() == 0 && uncaught() == 0)
      r |= 8;
  } catch (...) {
  }
  auto sees = reinterpret_cast<plugin_sees_t>(dlsym(h, "plugin_sees_current_exception"));
  auto term = reinterpret_cast<plugin_terminate_t>(dlsym(h, "plugin_terminate_handler"));
  auto rethrow = reinterpret_cast<plugin_rethrow_t>(dlsym(h, "plugin_rethrow"));
  int shared = 0;
  try {
    throw 7;
  } catch (int) {
    // Only a plugin that sees the handled exception can rethrow it; with two runtimes, `throw;`
    // in the plugin finds no exception and calls std::terminate (DECISIONS §2).
    if (sees())
      shared |= 1;
    try {
      if (shared & 1)
        rethrow();
    } catch (int v) {
      if (v == 7)
        shared |= 4;
    } catch (...) {
    }
  }
  std::set_terminate([] { std::_Exit(3); });
  if (term() == reinterpret_cast<void*>(std::get_terminate()))
    shared |= 2;
  struct probe {
    plugin_uncaught_t f;
    int* out;
    ~probe() { *out = f(); }
  };
  int during = -1;
  try {
    probe p{uncaught, &during};
    throw 1;
  } catch (int) {
  }
  if (during == 1)
    shared |= 8;
  std::printf("plugin %d shared-state %d\n", r, shared);
  return r == 15 ? 0 : 1;
}
