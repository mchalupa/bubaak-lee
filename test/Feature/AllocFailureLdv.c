// The error is reachable only through a failed calloc (as in
// ldv-regression/rule57_ebda_blast_2 of sv-benchmarks); the witnesses of the
// violation are written.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --error-fn=reach_error %t.bc > %t.log 2> %t.err
// RUN: not grep -q "KLEE: ERROR" %t.err
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --alloc-may-fail --error-fn=reach_error --write-witness --write-witness-yaml --write-unfinished-paths --dump-states-on-halt --write-incomplete-tests %t.bc > %t.log 2> %t.err
// RUN: grep -q "KLEE: ERROR: .*reach_error called" %t.err
// RUN: test -f %t.klee-out/test000002.assert.err
// RUN: test -f %t.klee-out/test000002.witness.yml
// RUN: test -f %t.klee-out/test000002.graphml
// RUN: grep -q "type: target" %t.klee-out/test000002.witness.yml
// RUN: not grep -q "__bbk_alloc_fail" %t.klee-out/test000002.witness.yml %t.klee-out/test000002.graphml
// RUN: %ktest-tool %t.klee-out/test000002.ktest | grep -A5 "name: '__bbk_alloc_fail'$" | grep -q "uint: 1$"
// (the failed allocation is the entry of the path)
// RUN: %paths show %t.klee-out/test000002.path | grep -qx "4294967295:0"
#include <stdlib.h>

void reach_error(void);
extern int __VERIFIER_nondet_int(void);

struct slot {
  int a;
  void *bus;
};

struct slot *tmp_slot;
int used_tmp_slot = 0;
int freed_tmp_slot = 1;

void *kzalloc(int size) { return calloc(1, size); }

void kfree(void *p) {
  if (p != 0 && p == tmp_slot)
    freed_tmp_slot = 1;
}

int controller(void) {
  tmp_slot = kzalloc(sizeof(*tmp_slot));
  if (!tmp_slot)
    return -2;
  used_tmp_slot = 0;
  freed_tmp_slot = 0;
  void *bus = kzalloc(16);
  if (!bus)
    goto error; // (tmp_slot is not used: the error)
  tmp_slot->bus = bus;
  used_tmp_slot = 1;
  if (__VERIFIER_nondet_int())
    goto error;
  return 0;
error:
  if (used_tmp_slot)
    kfree(tmp_slot);
  return -3;
}

int main(void) {
  controller();
  if (!used_tmp_slot && !freed_tmp_slot)
    reach_error();
  return 0;
}
