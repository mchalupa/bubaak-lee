// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s

// In the segment-based memory model each alloca gets a unique monotonically
// increasing segment id, so &a < &b is always concretely true (a is allocated
// before b).  No fork occurs and only one branch is taken.
#include "klee/klee.h"

int main() {
	int a,b;
	if (&a < &b) {
		klee_warning("First branch reached"); // CHECK: First branch
	} else {
		klee_warning("Second branch reached");
	}
	return 0;
}
