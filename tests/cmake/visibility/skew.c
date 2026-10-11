/* A process whose libycxx allocation table says another libycxx ABI version (DECISIONS §20.6): a C
   program defining and exporting __ycxx_allocation_functions, which every libycxx image then binds
   to, and loading the libycxx plugin argv[1]. The plugin's version check must stop the process at
   load time, naming both versions; prints "loaded" if it did not.
   The table is a weak definition, as in every libycxx image (src/runtime/new/allocation_table.cpp):
   on Darwin the images' references bind to the process's table by dyld's coalescing of weak
   definitions, which takes part only images that define weak symbols, so a strong definition in a
   program not linked against libycxx would be ignored and each plugin would keep its own table
   (DECISIONS §20.10, step 8). On ELF the program's definition wins either way. */
#include <dlfcn.h>
#include <stdio.h>

struct table { const char* abi; void* entries[20]; };
__attribute__((weak, visibility("default"))) const struct table __ycxx_allocation_functions = {"0.0-skewed", {0}};

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  void* h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!h) { printf("dlopen: %s\n", dlerror()); return 2; }
  printf("loaded\n");
  return 0;
}
