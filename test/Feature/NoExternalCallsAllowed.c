// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=none %t1.bc 2>&1 | FileCheck %s
// RUN: test %t.klee-out/test000001.user.err
#include <stdio.h>
#include <stdlib.h>

// abs() is lowered to the llvm.abs intrinsic by recent clang (so it is no
// longer an external call), and library functions such as printf are on KLEE's
// allow-list even under --external-calls=none. Use a genuinely external,
// non-allow-listed function to check that external calls are disallowed.
int external_function(int);

int main(int argc, char** argv) {
  // CHECK: Disallowed call to external function: external_function
  int x = external_function(argc);
  printf("%d\n", x);
  return 0;
}
