/*===-- __cxa_atexit.c ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include "klee/klee.h"

#include <unistd.h>

#define MAX_ATEXIT 128

static struct {
  void (*fn)(void*);
  void *arg;
  void *dso_handle;
} AtExit[MAX_ATEXIT];
static unsigned NumAtExit = 0;

/* Run the handlers in the reverse order of their registration. A handler is
 * removed before it is called, so that every handler runs at most once (e.g.,
 * if a handler calls exit()) and the handlers registered by the handlers run
 * too. */
static void RunAtExit(void) __attribute__((destructor));
static void RunAtExit(void) {
  while (NumAtExit > 0) {
    --NumAtExit;
    AtExit[NumAtExit].fn(AtExit[NumAtExit].arg);
  }
}

/* KLEE handles exit() only if the program does not define it. Returning
 * from main runs the handlers as a destructor, run them also on exit(). */
void exit(int status) {
  RunAtExit();
  _exit(status);
}

int __cxa_atexit(void (*fn)(void*),
                 void *arg,
                 void *dso_handle) {
  /* Better to just report an error here than return 1 (the defined
   * semantics).
   */
  if (NumAtExit == MAX_ATEXIT)
    klee_report_error(__FILE__,
                      __LINE__,
                      "__cxa_atexit: no room in array!",
                      "exec.err");
  
  AtExit[NumAtExit].fn = fn;
  AtExit[NumAtExit].arg = arg;
  ++NumAtExit;
  
  return 0;
}

// This variant is part of more recent glibc versions and
// is required by the Rust standard library
int __cxa_thread_atexit_impl(void (*fn)(void*), void *arg, void *dso_handle) {
  return __cxa_atexit(fn, arg, dso_handle);
}
