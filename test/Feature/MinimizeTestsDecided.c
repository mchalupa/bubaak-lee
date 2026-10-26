// The path constraints decide a value outside the small ranges (x == 123456,
// the range constraint simplifies to false): the test is still written
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=dfs %t.bc 2> %t.err
// RUN: %ktest-tool %t.klee-out/*.ktest | FileCheck %s
// CHECK: object 0: int : 123456

int __VERIFIER_nondet_int(void);

int main(void) {
  int x = __VERIFIER_nondet_int();
  int y = __VERIFIER_nondet_int();
  if (x == 123456)
    return y > 5;
  return 0;
}
