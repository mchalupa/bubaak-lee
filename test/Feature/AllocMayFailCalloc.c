// With -alloc-may-fail, calloc may return NULL; a successful calloc still
// returns zeroed memory.
// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -alloc-may-fail %t1.bc > %t.out 2> %t.err
// RUN: FileCheck --check-prefix=OUT %s < %t.out
// RUN: FileCheck --check-prefix=ERR %s < %t.err
// RUN: test -f %t.klee-out/test000001.assert.err
// RUN: not test -f %t.klee-out/test000002.ktest
// RUN: %ktest-tool %t.klee-out/test000001.ktest | FileCheck --check-prefix=KTEST %s
// RUN: rm -rf %t.klee-out-libc
// RUN: %klee --output-dir=%t.klee-out-libc -alloc-may-fail -libc=klee %t1.bc > %t.out 2> %t.err
// RUN: FileCheck --check-prefix=OUT %s < %t.out
// RUN: FileCheck --check-prefix=ERR %s < %t.err

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  char *p = calloc(4, 2);
  if (p == NULL) {
    assert(0 && "calloc failed");
  }
  for (int i = 0; i < 8; ++i)
    assert(p[i] == 0);
  free(p);
  printf("calloc succeeded\n");
  klee_silent_exit(0);
  return 0;
}

// OUT: calloc succeeded
// ERR-NOT: ASSERTION FAIL: p[i] == 0
// ERR: ASSERTION FAIL: 0 && "calloc failed"
// ERR-NOT: ASSERTION FAIL: p[i] == 0
// KTEST: num objects: 1
// KTEST: object 0: name: '__bbk_alloc_fail'
// KTEST-NEXT: object 0: size: 1
// KTEST: object 0: uint: {{[1-9][0-9]*}}
