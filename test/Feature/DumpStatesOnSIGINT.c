// Check that when KLEE is halted by SIGINT (ctrl-c) while it is in the middle
// of a solver query, the query is not cancelled (the solver must not handle
// SIGINT itself), KLEE does not crash and it writes tests for the remaining
// states. The states of this program keep exiting while the loop keeps going,
// so KLEE is busy generating tests (solver queries) most of the time.
//
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out %t.log
// RUN: sh -c '%klee --output-dir=%t.klee-out --use-forked-solver=false --dump-states-on-halt=true --write-incomplete-tests --max-time=60s %t.bc > %t.log 2>&1 & pid=$!; sleep 2; kill -INT $pid; wait $pid'
// RUN: FileCheck %s -input-file=%t.log
// RUN: ls %t.klee-out | FileCheck %s --check-prefix=CHECK-FILES

// CHECK: ctrl-c detected, requesting interpreter to halt
// CHECK-NOT: unable to compute initial values
// CHECK-NOT: Unhandled solver failure
// CHECK: halting execution, dumping remaining states
// CHECK-NOT: unable to compute initial values
// CHECK: KLEE: done: generated tests

// CHECK-FILES: .early

extern int __VERIFIER_nondet_int(void);

int main(void) {
  int x = __VERIFIER_nondet_int();
  int y = __VERIFIER_nondet_int();
  int z = __VERIFIER_nondet_int();
  while (y > 0 && x >= z && z <= 2147483647 - y) {
    z = z + y;
  }
  return 0;
}
