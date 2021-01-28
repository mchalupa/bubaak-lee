// REQUIRES: geq-llvm-10.0

// RUN: %clang %s -fsanitize=pointer-overflow -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdio.h>

int main() {
  size_t address;
  volatile char *result;

  klee_make_symbolic(&address, sizeof(address), "address");

  char *ptr = (char *)address;

  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: nullptr-after-nonzero-offset
  result = ptr + 1;
  return 0;
}
