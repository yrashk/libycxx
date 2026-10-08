// The plugin: allocates and frees standard containers for the host, and throws.
#include "api.hpp"
#include <exception>
#include <stdexcept>
std::vector<std::string>* plugin_make(const std::string& seed, int n) {
  auto* v = new std::vector<std::string>;
  for (int i = 0; i < n; ++i)
    v->push_back(seed + std::string(30, 'p') + std::to_string(i));
  return v;
}
std::size_t plugin_consume(std::vector<std::string>* v) {
  std::size_t total = 0;
  for (const auto& s : *v)
    total += s.size();
  delete v;
  return total;
}
void plugin_throw(const std::string& what) { throw std::runtime_error(what); }
int plugin_uncaught() { return std::uncaught_exceptions(); }
