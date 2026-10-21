// Unused arguments of main (only stored into their allocas at -O0) are not
// reported.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --external-calls=pure %t.bc 2>&1 | FileCheck %s
// RUN: %clang %s -emit-llvm %O0opt -g -DMAIN_VOID -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --external-calls=pure %t2.bc 2>&1 | FileCheck %s

// CHECK-NOT: under-approximation: main arguments
// CHECK: KLEE: done: completed paths = 1

#ifdef MAIN_VOID
int main(void) {
#else
int main(int argc, char **argv, char **envp) {
#endif
  return 0;
}
