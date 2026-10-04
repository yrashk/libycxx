// [except.handle]/3.3: a pointer handler matches by "a qualification conversion"; since
// C++20 [conv.qual]/3 a qualification conversion may also convert "array of N T" to "array
// of unknown bound of T" at any level (P0388), adding cv-qualifiers as usual. So a thrown
// int(*)[3] matches handlers of type int(*)[] and const int(*)[].
#include "check.hpp"

int main() {
  int arr[3] = {1, 2, 3};
  int which = 0;
  try {
    throw &arr;
  } catch (int (*p)[]) {
    which = (*p)[2];
  }
  CHECK(which == 3);

  which = 0;
  try {
    throw &arr;
  } catch (const int (*p)[]) {
    which = (*p)[1];
  }
  CHECK(which == 2);

  which = 0;
  try {
    throw &arr;
  } catch (const int (*p)[3]) {
    which = (*p)[0];
  }
  CHECK(which == 1);

  // a different bound is not a qualification conversion
  which = 0;
  try {
    try {
      throw &arr;
    } catch (int (*)[4]) {
      which = -1;
    }
  } catch (...) {
    which = 1;
  }
  CHECK(which == 1);
  return 0;
}
