// The size of an object of a symbolic size that is not minimized (no time
// left for the minimization) can be huge in the model, the test case has at
// most 1 MiB of the object (and KLEE does not crash writing it)
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --malloc-symbolic-contents --max-time=2s --dump-states-on-halt=1 --write-incomplete-tests --max-dump-minimization-time=1ms %t.bc 2> %t.err
// RUN: FileCheck --input-file=%t.err %s
// CHECK: the symbolic size of an object in a test case is {{[0-9]+}} bytes, writing only 1048576
// CHECK: KLEE: done: generated tests =

#include <stdlib.h>

extern unsigned long __VERIFIER_nondet_ulong(void);

int main(void) {
  unsigned long n = __VERIFIER_nondet_ulong();
  if (n < 16)
    return 0;
  char *p = malloc(n);
  if (!p)
    return 0;
  for (;;)
    if (p[0] == 1)
      p[0] = 2;
}
