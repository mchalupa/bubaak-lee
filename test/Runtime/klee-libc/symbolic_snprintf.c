// snprintf of symbolic integers: the length of the output is exact and the
// digits are the digits of the value.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --exit-on-error %t.bc 2>&1 | FileCheck %s

// CHECK-NOT: ERROR
// CHECK: KLEE: done

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  char buf[16];
  int x = klee_range(-99, 1000, "x");
  int n = snprintf(buf, sizeof(buf), "%d", x);
  assert(n == (int)strlen(buf));
  if (x >= 100)
    assert(n == 3 && buf[0] == '0' + x / 100);
  if (x < -9)
    assert(n == 3 && buf[0] == '-');
  return 0;
}
