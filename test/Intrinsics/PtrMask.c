// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t.bc

// Recent clang lowers pointer-alignment idioms (e.g. __builtin_align_down) to
// the llvm.ptrmask intrinsic. KLEE must lower it (IntrinsicCleaner turns it
// into inttoptr(and(ptrtoint(p), mask))); otherwise it aborts with
// "unimplemented intrinsic: llvm.ptrmask".
//
// NOTE: the segment-based memory model masks the in-object offset, so the
// absolute address of the result is not necessarily aligned (it is symbolic);
// only the offset-level properties checked below hold.

#include "klee/klee.h"

#include <assert.h>

int main() {
  char buf[64];
  char *p = buf + 5;

  char *aligned = (char *)__builtin_align_down(p, 8);

  // The aligned pointer is still in bounds and usable.
  *aligned = 42;
  assert(*aligned == 42);

  // Aligning down never moves the pointer forward and clears the low bits of
  // the offset (offset 5 -> 0, so aligned == buf and p - aligned == 5).
  assert(aligned <= p);
  assert((p - aligned) == 5);

  return 0;
}
