// The arguments of main are concrete, reading them is an under-approximation.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --external-calls=pure %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --external-calls=pure --main-args-underapprox=false %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK-OFF

// CHECK: KLEE: WARNING: under-approximation: main arguments
// CHECK-OFF-NOT: under-approximation: main arguments

int main(int argc, char **argv) {
  if (argc > 5)
    return 1;
  return 0;
}
