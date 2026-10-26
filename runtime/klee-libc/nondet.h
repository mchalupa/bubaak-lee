/*===-- nondet.h ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

/* Helpers for the models of the functions whose results depend on the
 * environment of the program (input, environment variables, time, random
 * numbers, ...).
 *
 * The models explore only behaviors that the real function can have (they
 * under-approximate), so that an error found through them is a real error.
 * A model that explores only a subset of the behaviors calls
 * klee_report_under_approximation, which prints (once per function)
 *   KLEE: WARNING: under-approximation: NAME
 * so that the exploration of all paths is not taken as a proof.
 *
 * The values are created by klee_make_symbolic (independently of the policy
 * for external calls), their names start with "__klee_model_". */

#ifndef KLEE_LIBC_NONDET_H
#define KLEE_LIBC_NONDET_H

#include "klee/klee.h"

#include <stddef.h>

#define KLEE_MODEL(name) "__klee_model_" name

static inline int __klee_libc_nondet_int(const char *name) {
  int x;
  klee_make_symbolic(&x, sizeof(x), name);
  return x;
}

static inline long __klee_libc_nondet_long(const char *name) {
  long x;
  klee_make_symbolic(&x, sizeof(x), name);
  return x;
}

static inline size_t __klee_libc_nondet_size(const char *name) {
  size_t x;
  klee_make_symbolic(&x, sizeof(x), name);
  return x;
}

/* Write symbolic bytes to dst[0..len), len may be symbolic (the writing
 * forks on it, only the bytes dst[0..len) are accessed). */
static inline void __klee_libc_write_symbolic(void *dst, size_t len,
                                              const char *name) {
  unsigned char tmp[64];
  unsigned char *d = (unsigned char *)dst;
  size_t i, k;
  for (i = 0; i < len; i += sizeof(tmp)) {
    klee_make_symbolic(tmp, sizeof(tmp), name);
    for (k = 0; k < sizeof(tmp) && i + k < len; ++k)
      d[i + k] = tmp[k];
  }
}

/* Stop exploring the path: the model cannot continue with only real
 * behaviors (reported as an under-approximation, the state is terminated
 * with a kill test, see klee_unsupported) */
static inline void __klee_libc_unsupported(const char *function) {
  klee_unsupported(function);
}

#endif
