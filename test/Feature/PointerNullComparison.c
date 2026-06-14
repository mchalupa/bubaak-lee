// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

// Tests that stack, global, and heap pointers are never equal to NULL and
// that NULL equals itself.  In the segment model every valid object has
// segment > 0 while NULL has segment 0, so all comparisons are decidable
// without solver queries.

#include "klee/klee.h"

#include <assert.h>
#include <stdlib.h>

int g;

int main() {
  int x;
  int *sp = &x;
  int *gp = &g;
  int *hp = malloc(sizeof(int));

  klee_assert(sp != NULL);
  klee_assert(gp != NULL);
  klee_assert(hp != NULL);

  int *np = NULL;
  klee_assert(np == NULL);
  klee_assert(sp != np);
  klee_assert(gp != np);
  klee_assert(hp != np);

  free(hp);
  return 0;
}
