// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -output-dir=%t.klee-out -write-witness-yaml -error-fn=reach_error -exit-on-error-type=Assert %t.bc
// RUN: FileCheck %s -input-file=%t.klee-out/test000001.witness.yml

// CHECK: - entry_type: violation_sequence
// CHECK: format_version: '2.0'
// CHECK: name: KLEE
// CHECK: content:
// CHECK: type: function_return
// CHECK: value: '\result == {{[0-9]+}}'
// CHECK: line: 34
// CHECK-NEXT: column: 16
// CHECK: type: function_return
// CHECK: value: '\result == -{{[0-9]+}}'
// CHECK: line: 35
// CHECK-NEXT: column: 11
// CHECK: type: target
// CHECK: line: 29
// CHECK-NEXT: column: 5
// CHECK-NEXT: function: "check"

void reach_error(void) { __builtin_trap(); }
extern unsigned __VERIFIER_nondet_uint(void);
extern int __VERIFIER_nondet_int(void);

void check(int c) {
  if (!c) {
    reach_error();
  }
}

int main(void) {
  unsigned n = __VERIFIER_nondet_uint();
  int x = __VERIFIER_nondet_int();
  if (n > 5 && x < -10)
    check(0);
  return 0;
}
