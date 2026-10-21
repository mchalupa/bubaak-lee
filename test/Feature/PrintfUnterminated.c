// A printed string that is not terminated in its object although all its
// bytes were written is a memory error (the printing function reads out of
// the object), the same for a pointer out of its object. A string that is
// not terminated only because of a byte that may not be initialized is not
// an error, but it is an under-approximation.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --external-calls=pure %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --external-calls=pure %t.bc 1 2>&1 | FileCheck %s -check-prefix=CHECK-UNINIT
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --external-calls=pure %t.bc 1 2 2>&1 | FileCheck %s -check-prefix=CHECK-BEFORE

// CHECK-NOT: under-approximation: printf
// CHECK: ERROR: {{.*}}PrintfUnterminated.c:34: memory error: out of bound pointer
// CHECK-UNINIT: under-approximation: printf
// CHECK-UNINIT-NOT: ERROR
// CHECK-BEFORE: ERROR: {{.*}}PrintfUnterminated.c:28: memory error: out of bound pointer

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  char buf[10];
  char big[11];
  if (argc == 3) {
    memcpy(buf, "AAAAAAAAAA", 10);
    printf("%s\n", buf - 1);
  } else if (argc == 2) {
    memcpy(big, "AAAAAAAAAA", 10); /* big[10] is not initialized */
    printf("%s\n", big);
  } else {
    memcpy(buf, "AAAAAAAAAA", 10);
    printf("%s\n", buf);
  }
  return 0;
}
