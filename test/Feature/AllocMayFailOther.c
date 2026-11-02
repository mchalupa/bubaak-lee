// The other allocation functions may fail with -alloc-may-fail where KLEE
// models them (aligned_alloc, memalign, strdup, strndup; posix_memalign is
// not modelled, it is an external call). This test tolerates both models
// (failing or not), but a failure must be NULL and a success a usable
// block: no memory errors, no crash of KLEE.
// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -alloc-may-fail -libc=klee %t1.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ > %t.files
// RUN: not grep -q "err" %t.files

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

int main(void) {
  char *s = strdup("abc");
  if (s) {
    if (s[0] != 'a' || s[3] != 0)
      abort();
    free(s);
  }
  char *t = strndup("abcdef", 2);
  if (t) {
    if (t[1] != 'b' || t[2] != 0)
      abort();
    free(t);
  }
  char *m = memalign(16, 64);
  if (m) {
    m[63] = 1;
    free(m);
  }
  char *a = aligned_alloc(16, 32);
  if (a) {
    a[31] = 1;
    free(a);
  }
  printf("done\n");
  return 0;
}

// CHECK-NOT: memory error
// CHECK: KLEE: done
