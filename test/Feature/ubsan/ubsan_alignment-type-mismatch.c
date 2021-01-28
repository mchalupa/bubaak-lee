// RUN: %clang %s -fsanitize=alignment -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdlib.h>

int main() {
  int x = klee_range(0, 5, "x");
  volatile int result;

  char c[] __attribute__((aligned(8))) = {0, 0, 0, 0, 1, 2, 3, 4, 5};
  int *p = (int *)&c[x];

  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: misaligned-pointer-use
  result = *p;
  return 0;
}
