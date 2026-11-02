// With -alloc-may-fail, realloc may return NULL: then the old block stays
// valid and unchanged (it is neither freed nor moved), so reading it and
// freeing it afterwards is not an error. free(NULL) is a no-op.
// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -alloc-may-fail --check-memcleanup %t1.bc > %t.out 2> %t.err
// RUN: FileCheck --check-prefix=OUT %s < %t.out
// RUN: FileCheck --check-prefix=ERR %s < %t.err
// (no memory error, no leak: only the one error test of the reached failure)
// RUN: ls %t.klee-out/ > %t.files
// RUN: not grep -q -E "ptr.err|free.err|leak.err" %t.files
// RUN: test -f %t.klee-out/test000001.assert.err
// RUN: not test -f %t.klee-out/test000002.ktest

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  char *p = malloc(4);
  if (p == NULL)
    klee_silent_exit(0);
  p[0] = 'a';
  p[3] = 'd';

  char *q = realloc(p, 1024);
  if (q == NULL) {
    // the old block is still there with its contents
    assert(p[0] == 'a' && p[3] == 'd');
    p[1] = 'b';
    free(p);
    free(NULL);
    assert(0 && "realloc failed");
  }
  assert(q[0] == 'a' && q[3] == 'd');
  q[1000] = 'x';
  free(q);
  printf("realloc succeeded\n");
  klee_silent_exit(0);
  return 0;
}

// OUT: realloc succeeded
// ERR-NOT: memory error
// ERR-NOT: ASSERTION FAIL: p[0] == 'a'
// ERR: ASSERTION FAIL: 0 && "realloc failed"
// ERR-NOT: memory error
