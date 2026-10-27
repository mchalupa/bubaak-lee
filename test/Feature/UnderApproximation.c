// Calls of undefined functions of the program are pure (no warning), calls
// of unmodelled functions of the C library are reported as
// under-approximations and their paths fail (unless
// --fail-unmodelled-libc-calls=false).
// Printing symbolic values is skipped, its result is nondet if it is used.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --external-calls=pure --fail-unmodelled-libc-calls=false %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --external-calls=pure %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK-FAIL

// CHECK-NOT: under-approximation: my_undefined
// CHECK-NOT: under-approximation: printf
// CHECK: KLEE: WARNING: under-approximation: strverscmp
// CHECK-NOT: failed external call: printf
// CHECK: completed paths = 2
// CHECK-FAIL: KLEE: WARNING: under-approximation: strverscmp
// CHECK-FAIL: ERROR: {{.*}}UnderApproximation.c:36: failed external call: strverscmp

#define _GNU_SOURCE
#include "klee/klee.h"

#include <stdio.h>
#include <string.h>

extern int my_undefined(int);

int main(void) {
  int x = klee_int("x");
  int y = my_undefined(x);
  int p;

  printf("%d\n", x); /* skipped */
  puts("concrete");  /* executed */

  p = strverscmp("abc", "b");

  if (printf("%d\n", y) > 3)
    return 1;
  return 0;
}
