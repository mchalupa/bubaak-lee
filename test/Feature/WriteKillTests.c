// A float concretization writes a kill test
// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --write-kill-tests %t1.bc > %t1.out 2> %t1.err
// RUN: FileCheck --check-prefix=CHECK-CONC --input-file=%t1.err %s
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000001.concretized
// CHECK-CONC: KLEE: WARNING: Wrote kill test: {{.*}}.klee-out/test000001.ktest (type: concretized, reason: floating point)

// A call of an undefined external function with a symbolic argument kills the state
// RUN: %clang %s -emit-llvm %O0opt -DEXTERNAL -c -o %t2.bc
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --write-kill-tests %t2.bc > %t2.out 2> %t2.err
// RUN: FileCheck --check-prefix=CHECK-EXT --input-file=%t2.err %s
// RUN: test -f %t.klee-out-2/test000001.ktest
// RUN: test -f %t.klee-out-2/test000001.external.err
// CHECK-EXT: KLEE: WARNING: Wrote kill test: {{.*}}.klee-out-2/test000001.ktest (type: external, reason: failed external call: external_function)

// Without the option, no kill test is written
// RUN: rm -rf %t.klee-out-3
// RUN: %klee --output-dir=%t.klee-out-3 %t1.bc > %t3.out 2> %t3.err
// RUN: not grep "Wrote kill test" %t3.err

#include "klee/klee.h"

extern int external_function(int);

int main() {
  int x;
  klee_make_symbolic(&x, sizeof x, "x");
#ifdef EXTERNAL
  if (x == 3)
    return external_function(x);
  return 0;
#else
  double d = x;
  return d > 10.0;
#endif
}
