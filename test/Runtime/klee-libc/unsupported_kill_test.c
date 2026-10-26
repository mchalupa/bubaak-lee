// A call of a function without a model ends the path with a kill test
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --write-kill-tests %t.bc > %t.out 2> %t.err
// RUN: FileCheck --check-prefix=CHECK-ERR --input-file=%t.err %s
// RUN: FileCheck --check-prefix=CHECK-OUT --input-file=%t.err %s
// RUN: ls %t.klee-out | grep -c 'external.err' | grep 1
// CHECK-ERR: KLEE: WARNING: under-approximation: sscanf
// CHECK-ERR: KLEE: WARNING: Wrote kill test: {{.*}}.ktest (type: external, reason: unsupported function: sscanf)
// CHECK-OUT: KLEE: done: completed paths = 1
// CHECK-OUT: KLEE: done: partially completed paths = 1

// Without the option, no kill test is written
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --libc=klee %t.bc > %t2.out 2> %t2.err
// RUN: not grep "Wrote kill test" %t2.err

#include "klee/klee.h"
#include <stdio.h>

int main() {
  int x;
  klee_make_symbolic(&x, sizeof x, "x");
  if (x == 3) {
    int v = 0;
    return sscanf("1 2", "%d %d", &v, &v);
  }
  return 0;
}
