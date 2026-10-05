// Shared declarations of except/handler_internal_linkage_types and its second unit.
#pragma once
#include <typeinfo>

struct Shared {  // external linkage, one definition in both units
  int s = 1;
  virtual ~Shared() = default;
};

void tu2_throw_local();
void tu2_throw_hidden();
void tu2_throw_hidden_ptr();
void tu2_throw_box();
Shared* tu2_make_hidden();
const std::type_info& tu2_local_type();
const std::type_info& tu2_box_type();
bool tu2_catches_local(void (*f)());
