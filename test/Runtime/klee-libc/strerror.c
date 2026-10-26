// strerror returns the messages of glibc, also for a symbolic errnum
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --exit-on-error-type=Assert %t.bc > %t.out 2> %t.err
// RUN: FileCheck %s -input-file=%t.err
// RUN: ls %t.klee-out | grep -c assert.err | grep 1
// CHECK: ASSERTION FAIL: 0 && "found the path"

#include "klee/klee.h"
#include <assert.h>
#include <string.h>
#include <errno.h>

int main() {
  int e;
  klee_make_symbolic(&e, sizeof e, "e");
  klee_assume(e >= 0 && e <= 133);
  const char *m = strerror(e);
  if (e == 0 && strcmp(m, "Success") != 0)
    klee_assert(0 && "wrong Success");
  if (e == ENOENT && strcmp(m, "No such file or directory") != 0)
    klee_assert(0 && "wrong ENOENT");
  if (e == 133 && strcmp(m, "Memory page has hardware error") != 0)
    klee_assert(0 && "wrong 133");
  if (e == EINTR && m[0] == 'I' && m[1] == 'n')
    klee_assert(0 && "found the path");
  return 0;
}
