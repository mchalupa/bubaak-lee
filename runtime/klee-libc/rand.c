/*===-- rand.c ------------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include "nondet.h"

#include <stdlib.h>

/* Pseudo-random numbers are an unknown input of the program: any value
 * in the range of the function (the seed is ignored). */

__attribute__((weak)) int rand(void) {
  int r = __klee_libc_nondet_int(KLEE_MODEL("rand"));
  klee_assume(r >= 0); /* RAND_MAX is INT_MAX in glibc */
  return r;
}

__attribute__((weak)) long random(void) {
  long r = __klee_libc_nondet_long(KLEE_MODEL("random"));
  klee_assume(r >= 0 & r <= 2147483647L);
  return r;
}

__attribute__((weak)) void srand(unsigned int seed) { (void)seed; }

__attribute__((weak)) void srandom(unsigned int seed) { (void)seed; }
