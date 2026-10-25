// __VERIFIER_nondet_memory makes the given bytes symbolic: the whole struct,
// a part of an object (the rest keeps its value), and the errors.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=pure %t.bc 2>&1 | FileCheck %s
// RUN: %ktest-tool %t.klee-out/*.ktest | FileCheck --check-prefix=KTEST %s

// CHECK-DAG: ASSERTION FAIL: 0 && "0"
// CHECK-DAG: ASSERTION FAIL: 0 && "1"
// CHECK-DAG: ASSERTION FAIL: 0 && "2"
// CHECK-DAG: memory error: out of bound pointer
// CHECK-DAG: KLEE: done: completed paths =

// the inputs of the tests (8 bytes of the struct, 4 of the part, 3 = 2 + 1)
// KTEST-DAG: name: '__VERIFIER_nondet_memory:{{.*}}'
// KTEST-DAG: size: 8
// KTEST-DAG: size: 4
// KTEST-DAG: size: 2
// KTEST-DAG: size: 1

#include <assert.h>
#include <stddef.h>

extern void __VERIFIER_nondet_memory(void *ptr, size_t size);
extern int __VERIFIER_nondet_int(void);

struct A {
  int x;
  int y;
};

struct B {
  int x;
  int y;
  int z;
};

int main(void) {
  int which = __VERIFIER_nondet_int();
  if (which == 0) {
    // the whole struct, the values must be able to differ
    struct A a = {0, 0};
    __VERIFIER_nondet_memory(&a, sizeof(a));
    if (a.x != a.y)
      assert(0 && "0");
  } else if (which == 1) {
    // a part of an object: y is symbolic, x and z are kept
    struct B b = {1, 0, 3};
    __VERIFIER_nondet_memory(&b.y, sizeof(b.y));
    if (b.x != 1 || b.z != 3)
      return 5; // not reachable
    if (b.y == 42)
      assert(0 && "1");
  } else if (which == 2) {
    // a size of 0 does nothing, a part of 3 bytes (the 4th byte is kept)
    int i = 0x7f000000;
    __VERIFIER_nondet_memory(&i, 0);
    if (i != 0x7f000000)
      return 6; // not reachable
    __VERIFIER_nondet_memory(&i, 3);
    if ((i >> 24) != 0x7f)
      return 7; // not reachable
    if ((i & 0xffffff) == 0x123456)
      assert(0 && "2");
  } else {
    // out of bounds
    int i = 0;
    __VERIFIER_nondet_memory(&i, 2 * sizeof(i));
  }
  return 0;
}
