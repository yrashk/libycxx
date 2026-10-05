// Shared by format/formatter_headers_across_tus and the translation units in this directory:
// what each translation unit reports. Includes no header that declares formatter.
#pragma once
#include <cstddef>
#include <string>
#include <typeinfo>

struct Facts {
  std::size_t size_int, size_double, size_cstr, size_string, size_char, size_wchar, size_ptr, size_ref;
  const std::type_info* ti_int;
  const std::type_info* ti_string;
  const std::type_info* ti_wchar;
  const std::type_info* ti_ref;  // nullptr when the translation unit lacks <vector>
};

// translation units with <format> (different orders of <format>, <vector>, <stack>, <queue>)
std::string tu_b_text();
std::wstring tu_b_wtext();
Facts tu_b_facts();
std::string tu_c_text();
std::wstring tu_c_wtext();
Facts tu_c_facts();
std::string tu_d_text();
std::wstring tu_d_wtext();
Facts tu_d_facts();
// translation units without <format>
Facts tu_e_facts();  // <vector> only
Facts tu_f_facts();  // <stack> and <queue> only
