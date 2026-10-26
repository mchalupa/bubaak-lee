// The nondet values of long long are modelled (not pure external calls)
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=pure %t.bc 2>&1 | FileCheck %s
// RUN: %ktest-tool %t.klee-out/*.ktest | FileCheck --check-prefix=CHECK-TEST %s

// CHECK-NOT: undefined function __VERIFIER_nondet_
// CHECK: KLEE: done: completed paths = 2
// CHECK-TEST-DAG: name: '__VERIFIER_nondet_ulonglong:{{.*}}'
// CHECK-TEST-DAG: name: '__VERIFIER_nondet_longlong:{{.*}}'
// CHECK-TEST-DAG: uint: 12345678901

unsigned long long __VERIFIER_nondet_ulonglong(void);
long long __VERIFIER_nondet_longlong(void);

int main(void) {
  unsigned long long u = __VERIFIER_nondet_ulonglong();
  long long l = __VERIFIER_nondet_longlong();
  if (u == 12345678901ULL)
    return 1;
  return l > 0 ? 0 : 2;
}
