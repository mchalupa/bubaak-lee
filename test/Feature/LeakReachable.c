// Memory that is reachable from globals (also through other heap objects
// and from static locals) at the end of the program is not a leak.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-leaks %t.bc 2>&1 | FileCheck %s
// RUN: %clang %s -DLEAK -emit-llvm %O0opt -g -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-leaks %t2.bc 2>&1 | FileCheck %s -check-prefix=CHECK-LEAK

// CHECK-NOT: memory leak detected
// CHECK: KLEE: done
// CHECK-LEAK: memory leak detected

#include <stdlib.h>

struct node {
  int v;
  struct node *next;
};

struct node *head;

static char *get(void) {
  static char *str;
  if (!str)
    str = malloc(29);
  return str;
}

int main(void) {
  struct node *a = malloc(sizeof *a);
  a->next = malloc(sizeof *a);
  a->next->next = 0;
  head = a;
  get();
#ifdef LEAK
  a->next->next = malloc(sizeof *a);
  a->next->next = 0; /* lost */
#endif
  return 0;
}
