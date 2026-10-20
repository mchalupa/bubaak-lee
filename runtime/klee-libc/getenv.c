/*===-- getenv.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include "nondet.h"

#include <stdlib.h>

/* The variable is not set (the environment is empty). This is a real
 * behavior, but only one of them (an under-approximation). */

__attribute__((weak)) char *getenv(const char *name) {
  (void)name;
  klee_report_under_approximation("getenv");
  return (char *)0;
}

__attribute__((weak)) char *secure_getenv(const char *name) {
  (void)name;
  klee_report_under_approximation("secure_getenv");
  return (char *)0;
}
