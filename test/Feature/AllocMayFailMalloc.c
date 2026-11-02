// With -alloc-may-fail, malloc may return NULL (SV-COMP 2027): both outcomes
// are explored. The failure is a 1-byte symbolic object `__bbk_alloc_fail`
// in the .ktest (nonzero = the allocation fails).
// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -alloc-may-fail %t1.bc > %t.out 2> %t.err
// RUN: FileCheck --check-prefix=OUT %s < %t.out
// RUN: FileCheck --check-prefix=ERR %s < %t.err
// (the successful path exits silently, so the only test is the failing one)
// RUN: test -f %t.klee-out/test000001.assert.err
// RUN: not test -f %t.klee-out/test000002.ktest
// RUN: %ktest-tool %t.klee-out/test000001.ktest | FileCheck --check-prefix=KTEST %s
// The same with the libc of KLEE (as bubaak runs KLEE)
// RUN: rm -rf %t.klee-out-libc
// RUN: %klee --output-dir=%t.klee-out-libc -alloc-may-fail -libc=klee %t1.bc > %t.out 2> %t.err
// RUN: FileCheck --check-prefix=OUT %s < %t.out
// RUN: FileCheck --check-prefix=ERR %s < %t.err
// RUN: test -f %t.klee-out-libc/test000001.assert.err
// -alloc-may-fail=false is the default behaviour
// RUN: rm -rf %t.klee-out-off
// RUN: %klee --output-dir=%t.klee-out-off -alloc-may-fail=false --exit-on-error %t1.bc 2>&1 | FileCheck --check-prefix=OFF %s

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  int *p = malloc(sizeof(int));
  if (p == NULL) {
    assert(0 && "malloc failed");
  }
  *p = 1;
  free(p);
  printf("malloc succeeded\n");
  klee_silent_exit(0);
  return 0;
}

// OUT: malloc succeeded
// ERR: ASSERTION FAIL: 0 && "malloc failed"
// KTEST: num objects: 1
// KTEST: object 0: name: '__bbk_alloc_fail'
// KTEST-NEXT: object 0: size: 1
// KTEST: object 0: uint: {{[1-9][0-9]*}}
// OFF-NOT: ASSERTION FAIL
// OFF: malloc succeeded
// OFF-NOT: ASSERTION FAIL
