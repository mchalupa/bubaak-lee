//===-- SeedInfo.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SEEDINFO_H
#define KLEE_SEEDINFO_H

#include "klee/Expr/Assignment.h"

#include <string>

extern "C" {
  struct KTest;
  struct KTestObject;
}

namespace klee {
  class ExecutionState;
  class TimingSolver;
  class MemoryObject;

  class SeedInfo {
  public:
    VectorAssignment assignment;
    KTest *input;
    unsigned inputPosition;
    std::set<struct KTestObject*> used;
    
  public:
    explicit
    SeedInfo(KTest *_input) : assignment(true),
                             input(_input),
                             inputPosition(0) {}
    
    KTestObject *getNextInput(const MemoryObject *mo,
                             bool byName, bool strict = false);
    /// Get the input for an object (e.g., a nondet value) of the given
    /// name and size (in bytes)
    KTestObject *getNextInput(const std::string &name, uint64_t size,
                             bool byName, bool strict = false);
    
  private:
    KTestObject *getNextInput(const std::string &name, uint64_t size,
                             bool constSize, bool byName, bool strict,
                             const void *warnId);

  public:
    /// Patch the seed so that condition is satisfied while retaining as
    /// many of the seed values as possible.
    void patchSeed(const ExecutionState &state, 
                   ref<Expr> condition,
                   TimingSolver *solver);
  };
}

#endif /* KLEE_SEEDINFO_H */
