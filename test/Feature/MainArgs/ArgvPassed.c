// Passing argv to a function counts as reading it.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --external-calls=pure %t.bc 2>&1 | FileCheck %s

// CHECK: KLEE: WARNING: under-approximation: main arguments

static int first(char **argv) { return argv[0] != 0; }

int main(int argc, char **argv) {
  return !first(argv);
}
