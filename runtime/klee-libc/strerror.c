/*===-- strerror.c --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include "nondet.h"

/* The messages are not modelled (their contents and lengths matter) */
char *strerror(int errnum) {
  (void)errnum;
  __klee_libc_unsupported("strerror");
  return (char *)0;
}
