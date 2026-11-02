// valid-deref: with -alloc-may-fail, dereferencing the unchecked result of
// malloc is a null-pointer dereference (on the path where malloc fails).
// Without the option, the program is memory safe.
// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -alloc-may-fail --exit-on-error-type=Ptr %t1.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ > %t.files
// RUN: grep -q "ptr.err" %t.files
// RUN: rm -rf %t.klee-out-off
// RUN: %klee --output-dir=%t.klee-out-off --exit-on-error-type=Ptr %t1.bc 2>&1 | FileCheck --check-prefix=OFF %s
// RUN: ls %t.klee-out-off/ > %t.files-off
// RUN: not grep -q "ptr.err" %t.files-off

#include <stdlib.h>

int main(void) {
  int *p = malloc(sizeof(int));
  *p = 42;
  int r = *p;
  free(p);
  return r - 42;
}

// CHECK: AllocMayFailNullDeref.c:18: memory error
// OFF-NOT: memory error
// OFF: KLEE: done: completed paths = 1
