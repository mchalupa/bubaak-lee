// The nondet values of long and pointers have the width of the target, a
// 64-bit value stored to a 32-bit variable was out of bounds.
// RUN: %clang %s -m32 -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --external-calls=pure %t.bc 2>&1 | FileCheck %s

// CHECK-NOT: memory error
// CHECK: KLEE: done: completed paths = 2

unsigned long __VERIFIER_nondet_ulong(void);
long __VERIFIER_nondet_long(void);
void *__VERIFIER_nondet_pointer(void);

int main(void) {
  unsigned long u = __VERIFIER_nondet_ulong();
  long l = __VERIFIER_nondet_long();
  void *p = __VERIFIER_nondet_pointer();
  if (u + l == 0)
    return 1;
  return p == 0;
}
