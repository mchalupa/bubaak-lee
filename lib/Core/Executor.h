//===-- Executor.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Class to perform actual execution, hides implementation details from external
// interpreter.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXECUTOR_H
#define KLEE_EXECUTOR_H

#include "ExecutionState.h"
#include "UserSearcher.h"

#include "klee/ADT/RNG.h"
#include "klee/Core/BranchTypes.h"
#include "klee/Core/Interpreter.h"
#include "klee/Core/TerminationTypes.h"
#include "klee/Expr/ArrayCache.h"
#include "klee/Expr/ArrayExprOptimizer.h"
#include "klee/Module/Cell.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/System/Time.h"

#include "llvm/ADT/Twine.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

struct KTest;

namespace llvm {
  class BasicBlock;
  class BranchInst;
  class CallInst;
  class LandingPadInst;
  class Constant;
  class ConstantExpr;
  class Function;
  class GlobalValue;
  class Instruction;
  class LLVMContext;
  class DataLayout;
  class Twine;
  class Value;
}

namespace klee {
class Array;
class Assignment;
struct Cell;
class ExecutionState;
class ExternalDispatcher;
class Expr;
class InstructionInfoTable;
class KCallable;
struct KFunction;
struct KInstruction;
class KInstIterator;
class KModule;
class MemoryManager;
class MemoryObject;
class ObjectState;
class ExecutionTree;
class Searcher;
class SeedInfo;
class SpecialFunctionHandler;
struct StackFrame;
class StatsTracker;
class TimingSolver;
class TreeStreamWriter;
class MergeHandler;
class MergingSearcher;
template <class T> class ref;

/// \todo Add a context object to keep track of data only live
/// during an instruction step. Should contain addedStates,
/// removedStates, and haltExecution, among others.

class Executor : public Interpreter {
  friend class OwningSearcher;
  friend class WeightedRandomSearcher;
  friend class SpecialFunctionHandler;
  friend class StatsTracker;
  friend class MergeHandler;
  friend class ObjectState;
  friend class ObjectStatePlane;
  friend klee::Searcher *klee::constructUserSearcher(Executor &executor);

public:
  typedef std::pair<ExecutionState*,ExecutionState*> StatePair;

  /// The random number generator.
  RNG theRNG;

private:
  std::unique_ptr<KModule> kmodule;
  InterpreterHandler *interpreterHandler;
  Searcher *searcher;

  ExternalDispatcher *externalDispatcher;
  std::unique_ptr<TimingSolver> solver;
  std::unique_ptr<MemoryManager> memory;
  std::set<ExecutionState*, ExecutionStateIDCompare> states;
  StatsTracker *statsTracker;
  TreeStreamWriter *pathWriter, *symPathWriter;
  SpecialFunctionHandler *specialFunctionHandler;
  TimerGroup timers;
  std::unique_ptr<ExecutionTree> executionTree;

  /// Used to track states that have been added during the current
  /// instructions step. 
  /// \invariant \ref addedStates is a subset of \ref states. 
  /// \invariant \ref addedStates and \ref removedStates are disjoint.
  std::vector<ExecutionState *> addedStates;
  /// Used to track states that have been removed during the current
  /// instructions step. 
  /// \invariant \ref removedStates is a subset of \ref states. 
  /// \invariant \ref addedStates and \ref removedStates are disjoint.
  std::vector<ExecutionState *> removedStates;

  /// When non-empty the Executor is running in "seed" mode. The
  /// states in this map will be executed in an arbitrary order
  /// (outside the normal search interface) until they terminate. When
  /// the states reach a symbolic branch then either direction that
  /// satisfies one or more seeds will be added to this map. What
  /// happens with other states (that don't satisfy the seeds) depends
  /// on as-yet-to-be-determined flags.
  std::map<ExecutionState*, std::vector<SeedInfo> > seedMap;

  /// Map of globals to their representative memory object.
  std::map<const llvm::GlobalValue*, MemoryObject*> globalObjects;

  /// Map of globals to their bound address. This also includes
  /// globals that have no representative object (e.g. aliases).
  std::map<const llvm::GlobalValue*, KValue> globalAddresses;

  /// Map of legal function addresses to the corresponding Function.
  /// Used to validate and dereference function pointers.
  std::unordered_map<std::uint64_t, llvm::Function*> legalFunctions;

  /// When non-null the bindings that will be used for calls to
  /// klee_make_symbolic in order replay.
  const struct KTest *replayKTest;

  // values for replaying nondet values -- for each object
  // with a given name, line, col and sequence number we have
  // a concrete value -- this is a sequence of the values
  // as they appear on the path being replayed
  std::vector<std::tuple<std::string, unsigned, unsigned,
                         ConcreteValue>> replayNondet;

  /// When non-null a list of branch decisions to be used for replay.
  const std::vector<bool> *replayPath;

  /// The index into the current \ref replayKTest or \ref replayPath
  /// object.
  unsigned replayPosition;

  /// When non-null a list of "seed" inputs which will be used to
  /// drive execution.
  const std::vector<struct KTest *> *usingSeeds;  

  /// Record the decisions at symbolic branches on the paths
  /// (-write-unfinished-paths, -prefix-paths)
  bool recordDecisions = false;

  /// The trie of the prefix paths (-prefix-paths)
  std::unique_ptr<PrefixPathNode> prefixPaths;

  /// The paths of the states that KLEE did not finish
  /// (-write-unfinished-paths)
  std::unique_ptr<llvm::raw_fd_ostream> unfinishedPathsFile;

  /// The paths of the states killed by an execution error
  /// (-write-unfinished-paths)
  std::unique_ptr<llvm::raw_fd_ostream> killedPathsFile;

  /// The prefix paths whose ends some states reached (-prefix-paths)
  std::unique_ptr<llvm::raw_fd_ostream> usedPrefixPathsFile;

  /// The number of states outside of the prefix paths
  std::uint64_t prunedStates = 0;

  /// The number of the prefix paths whose ends some states reached
  std::uint64_t usedPrefixPaths = 0;

  /// The number of the states that did not replay the prefix paths as the
  /// runs that wrote them (other branches)
  std::uint64_t misalignedPrefixPaths = 0;

  /// The paths of the states that are being dumped were written already
  bool unfinishedPathsWritten = false;

  /// Disables forking, instead a random path is chosen. Enabled as
  /// needed to control memory usage. \see fork()
  bool atMemoryLimit;

  /// Disables forking, set by client. \see setInhibitForking()
  bool inhibitForking;

  /// Signals the executor to halt execution at the next instruction
  /// step.
  bool haltExecution;  

  /// Whether the execution was halted because of -exit-on-error-type
  bool haltedOnError;

  /// Whether implied-value concretization is enabled. Currently
  /// false, it is buggy (it needs to validate its writes).
  bool ivcEnabled;

  /// The maximum time to allow for a single core solver query.
  /// (e.g. for a single STP query)
  time::Span coreSolverTimeout;

  /// The time limit for minimizing the nondeterministic values of a test case
  time::Span maxTestMinimizationTime;

  /// The time limit for minimizing the values of all the tests written when
  /// halting (0: no limit), the time spent so far, and whether the states are
  /// being dumped on halt
  time::Span maxDumpMinimizationTime;
  time::Span dumpMinimizationSpent;
  bool dumpingStates = false;

  /// A solver query failed (timed out) in a function that cannot terminate the
  /// state (e.g., toConstant, which must return a value). The state is
  /// terminated as soon as possible (when an instruction ends, or when
  /// it would terminate or fork otherwise) and the value it got is not used
  /// for any verdict.
  ExecutionState *solverFailedState = nullptr;

  /// The nondet values (value, segment) of the last test written by
  /// getSymbolicSolution, for the state \p lastTestState (getTestVector gives
  /// the same values as the .ktest file)
  const ExecutionState *lastTestState = nullptr;
  std::vector<std::pair<ref<ConstantExpr>, ref<ConstantExpr>>> lastTestNondets;
  std::string solverFailedMessage;

  /// Maximum time to allow for a single instruction.
  time::Span maxInstructionTime;

  /// Assumes ownership of the created array objects
  ArrayCache arrayCache;

  /// File to print executed instructions to
  std::unique_ptr<llvm::raw_ostream> debugInstFile;

  /// File to which the progress is written (--write-progress)
  std::unique_ptr<llvm::raw_fd_ostream> progressFile;

  /// The time when the execution started
  time::Point runStartTime;

  /// The time after which the execution halts (-max-time), 0 if none
  time::Span maxTime;

  // @brief Buffer used by logBuffer
  std::string debugBufferString;

  // @brief buffer to store logs before flushing to file
  llvm::raw_string_ostream debugLogBuffer;

  /// Optimizes expressions
  ExprOptimizer optimizer;

  /// Points to the merging searcher of the searcher chain,
  /// `nullptr` if merging is disabled
  MergingSearcher *mergingSearcher = nullptr;

  /// Typeids used during exception handling
  std::vector<ref<Expr>> eh_typeids;

  /// Return the typeid corresponding to a certain `type_info`
  ref<ConstantExpr> getEhTypeidFor(ref<Expr> type_info);

  llvm::Function* getTargetFunction(llvm::Value *calledVal);

  void executeInstruction(ExecutionState &state, KInstruction *ki);

  void run(ExecutionState &initialState);

  // Given a concrete object in our [klee's] address space, add it to 
  // objects checked code can reference.
  MemoryObject *addExternalObject(ExecutionState &state, void *addr, 
                                  unsigned size, bool isReadOnly,
                                  uint64_t specialSegment = 0);

  void initializeGlobalAlias(const llvm::Constant *c);
  void initializeGlobalObject(ExecutionState &state, ObjectState *os, 
			      const llvm::Constant *c,
			      unsigned offset);
  void initializeGlobals(ExecutionState &state);
  void allocateGlobalObjects(ExecutionState &state);
  void initializeGlobalAliases();
  void initializeGlobalObjects(ExecutionState &state);

  void stepInstruction(ExecutionState &state);
  void updateStates(ExecutionState *current);
  void transferToBasicBlock(llvm::BasicBlock *dst, 
			    llvm::BasicBlock *src,
			    ExecutionState &state);

  void callExternalFunction(ExecutionState &state,
                            KInstruction *target,
                            KCallable *callable,
                            const std::vector<Cell> &arguments);

  /// Report (once per function) that the behaviors of a function were not
  /// explored completely (KLEE: WARNING: under-approximation: NAME)
  static void reportUnderApproximation(const std::string &name);

  /// The result of checking a string argument of an external function
  /// (Invalid: the pointer points to an object that does not exist, e.g., a
  /// dead local variable or a freed object; OutOfBounds: the pointer is out
  /// of its object or the string is not terminated in the object on any
  /// path; Unterminated: the string is not terminated in the object, but
  /// it has bytes that may not be initialized)
  enum class StringCheck {
    Terminated,
    Unterminated,
    OutOfBounds,
    Unknown,
    Invalid
  };

  /// Check that the string at the address is terminated inside its object
  /// (if it is, its contents are stored to `contents` when concrete).
  StringCheck checkString(ExecutionState &state, const Cell &address,
                          std::string *contents, bool &concrete);

  /// Does the pointer have a constant segment of an ordinary object (memory
  /// on the stack, heap or a global) that is not alive (it was freed or it
  /// went out of scope)?
  bool pointsToDeadObject(ExecutionState &state, const KValue &pointer);

  /// Terminate the state with a memory error (use after free or out of bound
  /// pointer) for an access to the memory of a dead object
  void terminateStateOnDeadObject(ExecutionState &state,
                                  const KValue &pointer);

  /// Check the memory that a printing function (printf, puts, ...) reads.
  /// Sets `hasWrite` if the call writes memory (%n), `unknown` if the
  /// arguments could not be checked and `symbolicContents` if a printed
  /// string is symbolic or unterminated (it cannot be printed natively).
  /// Returns false if the state was terminated.
  bool checkOutputCall(ExecutionState &state, const std::string &name,
                       const std::vector<Cell> &arguments, bool &hasWrite,
                       bool &unknown, bool &symbolicContents);

  /// Bind a nondet result of a call of a printing function that is not
  /// executed (see -output-calls)
  void bindOutputCallResult(ExecutionState &state, KInstruction *target,
                            const std::string &name,
                            const std::vector<Cell> &arguments);

  ObjectState *bindObjectInState(ExecutionState &state, const MemoryObject *mo,
                                 bool isLocal, const Array *array = 0);

  /// Resolve a pointer to the memory objects it could point to the
  /// start of, forking execution when necessary and generating errors
  /// for pointers to invalid locations (either out of bounds or
  /// address inside the middle of objects).
  ///
  /// \param results[out] A list of ((MemoryObject,ObjectState),
  /// state) pairs for each object the given address can point to the
  /// beginning of.
  typedef std::vector< std::pair<std::pair<const MemoryObject*, const ObjectState*>, 
                                 ExecutionState*> > ExactResolutionList;
  void resolveExact(ExecutionState &state,
                    const KValue &address,
                    ExactResolutionList &results,
                    const std::string &name);

  /// Allocate and bind a new object in a particular state. NOTE: This
  /// function may fork.
  ///
  /// \param isLocal Flag to indicate if the object should be
  /// automatically deallocated on function return (this also makes it
  /// illegal to free directly).
  ///
  /// \param target Value at which to bind the base address of the new
  /// object.
  ///
  /// \param reallocFrom If non-zero and the allocation succeeds,
  /// initialize the new object from the given one and unbind it when
  /// done (realloc semantics). The initialized bytes will be the
  /// minimum of the size of the old and new objects, with remaining
  /// bytes initialized as specified by zeroMemory.
  ///
  /// \param allocationAlignment If non-zero, the given alignment is
  /// used. Otherwise, the alignment is deduced via
  /// Executor::getAllocationAlignment
  MemoryObject* executeAlloc(ExecutionState &state,
                             ref<Expr> size,
                             bool isLocal,
                             KInstruction *target,
                             bool zeroMemory=false,
                             const ObjectState *reallocFrom=nullptr,
                             size_t allocationAlignment=0);

  ref<Expr> getSizeForAlloca(ExecutionState& state, KInstruction *ki) const;

  void executeLifetimeIntrinsic(ExecutionState &state,
                                KInstruction *ki,
                                const std::vector<Cell> &arguments,
                                bool isEnd);

  void executeLifetimeIntrinsic(ExecutionState &state,
                                KInstruction *ki,
                                KInstruction *allocSite,
                                const KValue& address,
                                bool isEnd);

  /// Free the given address with checking for errors. If target is
  /// given it will be bound to 0 in the resulting states (this is a
  /// convenience for realloc). Note that this function can cause the
  /// state to fork and that \ref state cannot be safely accessed
  /// afterwards.
  void executeFree(ExecutionState &state,
                   const KValue &address,
                   KInstruction *target = 0);

  /// Serialize a landingpad instruction so it can be handled by the
  /// libcxxabi-runtime
  MemoryObject *serializeLandingpad(ExecutionState &state,
                                    const llvm::LandingPadInst &lpi,
                                    bool &stateTerminated);

  /// Unwind the given state until it hits a landingpad. This is used
  /// for exception handling.
  void unwindToNextLandingpad(ExecutionState &state);

  void executeCall(ExecutionState &state, 
                   KInstruction *ki,
                   llvm::Function *f,
                   const std::vector<Cell> &arguments);

  void executeMemoryRead(ExecutionState &state,
                         const KValue &address,
                         KInstruction *target);
  void executeMemoryWrite(ExecutionState &state,
                          const KValue &address,
                          const KValue &value);
  // do address resolution / object binding / out of bounds checking
  // and perform the operation
  void executeMemoryOperation(ExecutionState &state,
                              bool isWrite,
                              KValue address,
                              KValue value, /* undef if read */
                              KInstruction *target /* undef if write */);

  void executeMakeSymbolic(ExecutionState &state, const MemoryObject *mo,
                           const std::string &name);

  void executeMakeConcrete(ExecutionState &state,
                           const MemoryObject *mo,
                           const std::vector<unsigned char>& data);

  /// Create a new state where each input condition has been added as
  /// a constraint and return the results. The input state is included
  /// as one of the results. Note that the output vector may include
  /// NULL pointers for states which were unable to be created.
  void branch(ExecutionState &state, const std::vector<ref<Expr>> &conditions,
              std::vector<ExecutionState *> &result, BranchType reason);

  /// Fork current and return states in which condition holds / does
  /// not hold, respectively. One of the states is necessarily the
  /// current state, and one of the states may be null.
  StatePair fork(ExecutionState &current, ref<Expr> condition, bool isInternal,
                 BranchType reason);

  // If the MaxStatic*Pct limits have been reached, concretize the condition and
  // return it. Otherwise, return the unmodified condition.
  ref<Expr> maxStaticPctChecks(ExecutionState &current, ref<Expr> condition);

  /// Add the given (boolean) condition as a constraint on state. This
  /// function is a wrapper around the state's addConstraint function
  /// which also manages propagation of implied values,
  /// validity checks, and seed patching.
  void addConstraint(ExecutionState &state, ref<Expr> condition);

  // Called on [for now] concrete reads, replaces constant with a symbolic
  // Used for testing.
  ref<Expr> replaceReadWithSymbolic(ExecutionState &state, ref<Expr> e);

  KValue createNondetValue(ExecutionState &state,
                           unsigned size, bool isSigned,
                           KInstruction *instr,
                           const std::string &name,
                           bool isPointer = false);

  const Cell& eval(KInstruction *ki, unsigned index, 
                   ExecutionState &state) const;

  Cell& getArgumentCell(ExecutionState &state,
                        KFunction *kf,
                        unsigned index) {
    return state.stack.back().locals[kf->getArgRegister(index)];
  }

  Cell& getDestCell(ExecutionState &state,
                    KInstruction *target) {
    return state.stack.back().locals[target->dest];
  }

  void bindLocal(KInstruction *target,
                 ExecutionState &state,
                 const KValue &value);
  void bindArgument(KFunction *kf,
                    unsigned index,
                    ExecutionState &state,
                    const KValue &value);

  /// Evaluates an LLVM constant expression.  The optional argument ki
  /// is the instruction where this constant was encountered, or NULL
  /// if not applicable/unavailable.
  KValue evalConstantExpr(const llvm::ConstantExpr *c,
                          const KInstruction *ki = NULL);

  /// Evaluates an LLVM constant.  The optional argument ki is the
  /// instruction where this constant was encountered, or NULL if
  /// not applicable/unavailable.
  KValue evalConstant(const llvm::Constant *c,
                      const KInstruction *ki = NULL);

  /// Return a unique constant value for the given expression in the
  /// given state, if it has one (i.e. it provably only has a single
  /// value). Otherwise return the original expression.
  ref<Expr> toUnique(const ExecutionState &state, const ref<Expr> &e);

  /// Return a constant value for the given expression.  If \param concretize is
  /// true, the expression is forced to be a constant by adding a constraint if
  /// necessary. Note that this function breaks completeness and
  /// should generally be avoided.
  ///
  /// \param reason A documentation string stating the reason for concretization
  ref<klee::ConstantExpr> toConstant(ExecutionState &state, ref<Expr> e,
                                     const std::string &reason,
                                     bool concretize = true);

  /// Evaluate the given expression under each seed, and return the
  /// first one that results in a constant, if such a seed exist.  Otherwise,
  /// return the non-constant evaluation of the expression under one of the
  /// seeds.
  ref<klee::ConstantExpr> getValueFromSeeds(ExecutionState &state, ref<Expr> e);

  /// Bind a constant value for e to the given target. NOTE: This
  /// function may fork state if the state has multiple seeds.
  void executeGetValue(ExecutionState &state, const KValue& e, KInstruction *target);

  /// Get textual information regarding a memory address.
  std::string getKValueInfo(ExecutionState &state, const KValue &address) const;

  // Determines the \param lastInstruction of the \param state which is not KLEE
  // internal and returns its InstructionInfo
  const InstructionInfo & getLastNonKleeInternalInstruction(const ExecutionState &state,
      llvm::Instruction** lastInstruction);

  /// Remove state from queue and delete state. This function should only be
  /// used in the termination functions below.
  void terminateState(ExecutionState &state, StateTerminationType reason);

  /// Remove the state like terminateState, but do not count it as an
  /// explored path (it is outside of the prefix paths)
  void pruneState(ExecutionState &state);

  /// Remove state from queue and delete state (without counting it)
  void removeState(ExecutionState &state, StateTerminationType reason);

  /// Add the prefix paths from the file to the trie (-prefix-paths,
  /// -seeded-prefix-paths if `seeded`)
  void loadPrefixPaths(const std::string &file, bool seeded);

  /// Move the state to the node of the trie of the prefix paths; at the end
  /// of a prefix path, the state is free (and the prefix paths below are
  /// reported as used). False if the state must be pruned: it reached the
  /// end of a prefix path by other branches than the run that wrote the
  /// path, or it has no seeds and only seeded prefix paths end here
  bool enterPrefixNode(ExecutionState &state, const PrefixPathNode *node);

  /// Count a state that did not replay a prefix path (warn the first time)
  void noteMisaligned();

  /// May the state take the decision (it is free or a prefix path takes it)?
  static bool prefixAllows(const ExecutionState &state,
                           std::uint32_t decision) {
    return !state.prefixNode || state.prefixNode->children.count(decision);
  }

  /// Record the decision at a symbolic branch with the condition of the
  /// given hash (it must be allowed, see prefixAllows); the state is free
  /// when it reaches the end of a prefix path (the prefix paths below are
  /// reported as used). False if the state reached the end of a prefix path
  /// by other branches than the run that wrote the path (it must be pruned)
  bool decide(ExecutionState &state, std::uint32_t decision,
              unsigned conditionHash);

  /// Report the prefix paths of the subtree as used
  void reportUsedPrefixPaths(const PrefixPathNode &node);

  /// Write the path of the state (and the decision at the branch with the
  /// condition of the given hash, if not negative) to the unfinished paths
  /// (-write-unfinished-paths)
  void writeUnfinishedPath(const ExecutionState &state,
                           std::int64_t decision = -1,
                           unsigned conditionHash = 0);

  /// Call exit handler and terminate state normally
  /// (end of execution path)
  void terminateStateOnExit(ExecutionState &state);

  /// Call exit handler and terminate state early
  /// (e.g. due to state merging or memory pressure)
  void terminateStateEarly(ExecutionState &state, const llvm::Twine &message,
                           StateTerminationType reason);

  /// Call exit handler and terminate state early
  /// (e.g. caused by the applied algorithm as in state merging or replaying)
  void terminateStateEarlyAlgorithm(ExecutionState &state,
                                    const llvm::Twine &message,
                                    StateTerminationType reason);

  /// Call exit handler and terminate state early
  /// (e.g. due to klee_silent_exit issued by user)
  void terminateStateEarlyUser(ExecutionState &state,
                               const llvm::Twine &message);

  /// Call error handler and terminate state in case of errors.
  /// The underlying function of all error-handling termination functions
  /// below. This function should only be used in the termination functions
  /// below.
  void terminateStateOnError(ExecutionState &state,
                                    const llvm::Twine &message,
                                    StateTerminationType reason,
                                    const llvm::Twine &longMessage = "",
                                    const char *suffix = nullptr);

  /// Call error handler and terminate state in case of program errors
  /// (e.g. free()ing globals, out-of-bound accesses)
  void terminateStateOnProgramError(ExecutionState &state,
                                    const llvm::Twine &message,
                                    StateTerminationType reason,
                                    const llvm::Twine &longMessage = "",
                                    const char *suffix = nullptr);

  /// Call error handler and terminate state in case of execution errors
  /// (things that should not be possible, like illegal instruction or
  /// unlowered intrinsic, or unsupported intrinsics, like inline assembly)
  void terminateStateOnExecError(
      ExecutionState &state, const llvm::Twine &message,
      StateTerminationType = StateTerminationType::Execution);

  /// Remember that a solver query of \p state failed, see solverFailedState.
  void noteSolverFailure(ExecutionState &state, const std::string &message);

  /// If a solver query of \p state failed (noteSolverFailure), terminate the
  /// state as killed by the solver and return true. With nullptr, handle the
  /// failure of any state.
  bool terminateOnSolverFailure(ExecutionState *state);

  /// A model of the constraints of \p state to get the values of its test
  /// when the query for the model fails or times out: a seed of the state,
  /// or the model of the longest non-empty prefix of the constraints found
  /// in time (unless \p quick). nullptr if there is none: no test is written
  /// (the values must not be invented).
  std::shared_ptr<const Assignment> getFallbackModel(const ExecutionState &state,
                                                     bool quick);

  /// Call error handler and terminate state in case of solver errors
  /// (solver error or timeout)
  void terminateStateOnSolverError(ExecutionState &state,
                                   const llvm::Twine &message);

  /// Call error handler and terminate state for user errors
  /// (e.g. wrong usage of klee.h API)
  void terminateStateOnUserError(ExecutionState &state,
                                 const llvm::Twine &message,
                                 bool writeErr = true);

  /// bindModuleConstants - Initialize the module constant table.
  void bindModuleConstants();

  template <typename TypeIt>
  void computeOffsetsSeqTy(KGEPInstruction *kgepi,
                           ref<ConstantExpr> &constantOffset, uint64_t index,
                           const TypeIt it);

  template <typename TypeIt>
  void computeOffsets(KGEPInstruction *kgepi, TypeIt ib, TypeIt ie);

  /// bindInstructionConstants - Initialize any necessary per instruction
  /// constant values.
  void bindInstructionConstants(KInstruction *KI);

  void doImpliedValueConcretization(ExecutionState &state,
                                    ref<Expr> e,
                                    ref<ConstantExpr> value);

  bool getReachableMemoryObjects(ExecutionState &state,
                                 std::set<const MemoryObject *>&);

  void reportError(const llvm::Twine &message, const ExecutionState &state,
                   const llvm::Twine &info, const char *suffix,
                   enum StateTerminationType terminationType,
                   bool killTest = false);

  /// check memory usage and terminate states when over threshold of -max-memory + 100MB
  /// \return true if below threshold, false otherwise (states were terminated)
  bool checkMemoryUsage();

  /// check if branching/forking is allowed
  bool branchingPermitted(const ExecutionState &state) const;

  void printDebugInstructions(ExecutionState &state);
  void doDumpStates();

  /// Append the current progress to the progress file
  void writeProgress();

  /// With -write-incomplete-tests, write a test for the branch (given by
  /// `condition`) that is not explored because forking is inhibited
  /// (`decision` is the decision of the dropped branch, see decide)
  void writeDroppedBranchTest(const ExecutionState &state, std::int64_t decision,
                              ref<Expr> condition, unsigned conditionHash);

  /// Only for debug purposes; enable via debugger or klee-control
  void dumpStates();
  void dumpExecutionTree();

public:
  Executor(llvm::LLVMContext &ctx, const InterpreterOptions &opts,
      InterpreterHandler *ie);
  virtual ~Executor();

  const InterpreterHandler& getHandler() {
    return *interpreterHandler;
  }

  void setPathWriter(TreeStreamWriter *tsw) override { pathWriter = tsw; }

  void setSymbolicPathWriter(TreeStreamWriter *tsw) override {
    symPathWriter = tsw;
  }

  void setReplayKTest(const struct KTest *out) override {
    assert(!replayPath && "cannot replay both buffer and path");
    replayKTest = out;
    replayPosition = 0;
  }

  void setReplayPath(const std::vector<bool> *path) override {
    assert(!replayKTest && "cannot replay both buffer and path");
    replayPath = path;
    replayPosition = 0;
  }

  void setReplayNondet(const struct KTest *out) override;

  llvm::Module *setModule(std::vector<std::unique_ptr<llvm::Module>> &modules,
                          const ModuleOptions &opts) override;

  void useSeeds(const std::vector<struct KTest *> *seeds) override {
    usingSeeds = seeds;
  }

  void runFunctionAsMain(llvm::Function *f, int argc, char **argv,
                         char **envp) override;

  /*** Runtime options ***/

  void setHaltExecution(bool value) override { haltExecution = value; }

  void setInhibitForking(bool value) override { inhibitForking = value; }

  void prepareForEarlyExit() override;

  /*** State accessor methods ***/

  unsigned getPathStreamID(const ExecutionState &state) override;

  unsigned getSymbolicPathStreamID(const ExecutionState &state) override;

  void getConstraintLog(const ExecutionState &state, std::string &res,
                        Interpreter::LogType logFormat =
                            Interpreter::STP) override;

  bool getSymbolicSolution(
      const ExecutionState &state,
      std::vector<std::pair<std::string, std::vector<unsigned char>>> &res)
      override;

  /// The minimal (unsigned) value of `e` under `constraints`, false if it is
  /// not found before `deadline`.
  /// Make the values in \p nondets (a model) smaller, within \p deadline;
  /// the constraints of the values found are added to \p cm (which changes
  /// \p constraints). False if the deadline passed before all values were
  /// minimal (\p nondets is always valid).
  bool minimizeNondetValues(
      const ExecutionState &state, const ConstraintSet &constraints,
      ConstraintManager &cm, time::Point deadline,
      std::vector<std::pair<ref<ConstantExpr>, ref<ConstantExpr>>> &nondets);

  bool getMinimalValue(const ConstraintSet &constraints, ref<Expr> e,
                       time::Point deadline, SolverQueryMetaData &metaData,
                       ref<ConstantExpr> &result);

  // get a sequence of inputs that drive the program to this state
  std::vector<NamedConcreteValue>
  getTestVector(const ExecutionState &state) override;

  void getCoveredLines(const ExecutionState &state,
                       std::map<const std::string *, std::set<unsigned>> &res)
      override;

  Expr::Width getWidthForLLVMType(llvm::Type *type) const;
  size_t getAllocationAlignment(const llvm::Value *allocSite) const;

  /// Returns the errno location in memory of the state
  int *getErrnoLocation(const ExecutionState &state) const;

  MergingSearcher *getMergingSearcher() const { return mergingSearcher; };
  void setMergingSearcher(MergingSearcher *ms) { mergingSearcher = ms; };
};

} // namespace klee

#endif /* KLEE_EXECUTOR_H */
