// The interface of the shared library of linkage/shared_library_replaced_new.pass.cpp and
// linkage/shared_library_allocation_exchange.pass.cpp. No library type appears in a signature:
// GCC gives a function the visibility of its parameter and return types, and the library's types
// are hidden (DECISIONS §2), so objects are built and destroyed in the caller's storage.
#pragma once

// Allocates and frees inside the shared library: every form of the replaceable functions.
void lib_allocate_and_free();
// Construct in `storage` a std::string of 200 'x' / a std::vector<int> of 100 sevens, allocated
// in the shared library, for the caller to destroy.
void lib_make_string(void* storage);
void lib_make_vector(void* storage);
// Destroy (in the shared library) a std::string / std::vector<int> the caller built in `storage`.
void lib_destroy_string(void* storage);
void lib_destroy_vector(void* storage);
