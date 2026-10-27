// The result of printing symbolic values (not executed natively) is any
// admissible value (also an error), or a value of a success with
// --output-call-result=success
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=pure %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK-ADM
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=pure --output-call-result=success %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK-SUCC
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=pure --malloc-symbolic-contents --skip-unchecked-output %t.bc unterminated 2>&1 | FileCheck %s -check-prefix=CHECK-ADM
// CHECK-ADM-NOT: failed external call
// CHECK-ADM-NOT: symbolic-sized object
// CHECK-ADM: completed paths = 4
// CHECK-ADM-DAG: puts failed
// CHECK-ADM-DAG: putchar failed
// CHECK-ADM-DAG: printf failed
// CHECK-SUCC-NOT: failed
// CHECK-SUCC: completed paths = 1
#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  unsigned n;
  klee_make_symbolic(&n, sizeof n, "n");
  klee_assume(n >= 2 & n < 16);
  char *p = malloc(n);
  if (argc < 2) {
    p[0] = klee_range(1, 128, "c"); // symbolic contents
    p[1] = '\0';
  }
  if (puts(p) == EOF) {
    printf("puts failed\n");
    return 1;
  }
  int c = klee_range(1, 128, "d");
  if (putchar(c) != c) {
    printf("putchar failed\n");
    return 2;
  }
  if (printf("%d\n", c) < 0) {
    printf("printf failed\n");
    return 3;
  }
  return 0;
}
