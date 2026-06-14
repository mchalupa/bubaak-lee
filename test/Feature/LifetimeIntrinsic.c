// RUN: %clang %s -emit-llvm -g -O1 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t.bc

// At -O1 clang emits llvm.lifetime.start/end markers around scoped locals.
// KLEE executes these (executeLifetimeIntrinsic) and must read the marked
// pointer from the correct argument. LLVM 22 dropped the size argument from
// the lifetime intrinsics, moving the pointer from argument 1 to argument 0;
// using the wrong index aborts with "Unhandled argument for lifetime
// intrinsic". This test exercises that path on both a scalar and an array
// alloca.

#include "klee/klee.h"

#include <assert.h>

__attribute__((noinline)) void use(int *p) { *p = 42; }

int main() {
  {
    int x;
    use(&x);
    assert(x == 42);
  }
  {
    int arr[4];
    use(arr);
    assert(arr[0] == 42);
  }
  return 0;
}
