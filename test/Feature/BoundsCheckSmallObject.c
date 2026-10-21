// An access larger than the object is out of bounds even at offset 0.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --external-calls=pure %t.bc 2>&1 | FileCheck %s

// CHECK: BoundsCheckSmallObject.c:[[@LINE+6]]: memory error: out of bound pointer

#include <stdlib.h>

int main(void) {
  char *p = malloc(1);
  int x = *(int *)p;
  free(p);
  return x;
}
