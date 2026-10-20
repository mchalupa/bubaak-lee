// Errors inside the functions of KLEE's runtime (here memmove) are reported
// at the location of the call in the program: on stderr, in the .err file and
// in the witnesses.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --write-witness --write-witness-yaml --exit-on-error-type=Ptr %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK-STDERR
// RUN: FileCheck %s -check-prefix=CHECK-ERR -input-file=%t.klee-out/test000001.ptr.err
// RUN: FileCheck %s -check-prefix=CHECK-YAML -input-file=%t.klee-out/test000001.witness.yml
// RUN: FileCheck %s -check-prefix=CHECK-GRAPHML -input-file=%t.klee-out/test000001.graphml

// CHECK-STDERR: KLEE: ERROR: {{.*}}ErrorLocationInRuntime.c:34: memory error: out of bound pointer
// CHECK-ERR: File: {{.*}}ErrorLocationInRuntime.c
// CHECK-ERR-NEXT: Line: 34
// CHECK-ERR: Stack:
// CHECK-ERR-NEXT: memmove
// CHECK-ERR-NEXT: copy
// CHECK-YAML: type: target
// CHECK-YAML: line: 34
// CHECK-YAML-NEXT: column: 3
// CHECK-YAML-NEXT: function: "copy"
// CHECK-GRAPHML: <data key="violation">true</data>
// CHECK-GRAPHML: <edge source="{{[0-9]+}}" target="{{[0-9]+}}">
// CHECK-GRAPHML-NEXT: <data key="startline">34</data>

#include <string.h>

extern unsigned __VERIFIER_nondet_uint(void);

void copy(unsigned n) {
  int src[10] = {0};
  int dst[5];

  // the error
  memmove(dst, src, n * sizeof(int));
}

int main(void) {
  unsigned n = __VERIFIER_nondet_uint();
  if (n <= 10)
    copy(n);
  return 0;
}
