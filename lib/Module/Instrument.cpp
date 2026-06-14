//===-- Instrument.cpp ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// New-PassManager based implementation of the module preparation passes used
// for LLVM >= 17, where the legacy pass-creation API for the built-in
// transforms is no longer available.  KLEE's own passes are still legacy
// passes and are therefore run through a legacy::PassManager (the legacy pass
// infrastructure itself is still present in LLVM), while the built-in
// transforms (Scalarizer, LowerAtomic) are run through the new PassManager.
//
//===----------------------------------------------------------------------===//

#include "ModuleHelper.h"

#include "Passes.h"
#include "klee/Support/CompilerWarning.h"
#include "klee/Support/ErrorHandling.h"

DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar/LowerAtomicPass.h"
#include "llvm/Transforms/Scalar/Scalarizer.h"
DISABLE_WARNING_POP

using namespace llvm;

void klee::instrument(bool CheckDivZero, bool CheckOvershift,
                      llvm::Module *module) {
  // Inject checks prior to optimization... we also perform the
  // invariant transformations that we will end up doing later so that
  // optimize is seeing what is as close as possible to the final
  // module.

  // RaiseAsmPass is a KLEE legacy pass.
  {
    legacy::PassManager pm;
    pm.add(new klee::RaiseAsmPass());
    pm.run(*module);
  }

  // This pass will scalarize as much code as possible so that the Executor
  // does not need to handle operands of vector type for most instructions
  // other than InsertElementInst and ExtractElementInst.
  //
  // NOTE: Must come before division/overshift checks because those passes
  // don't know how to handle vector instructions.
  //
  // LowerAtomic replaces atomic instructions with non-atomic operations.
  // These are built-in transforms only available through the new PassManager.
  {
    PassBuilder PB;
    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    FunctionPassManager FPM;
    FPM.addPass(ScalarizerPass());
    FPM.addPass(LowerAtomicPass());

    ModulePassManager MPM;
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
    MPM.run(*module, MAM);
  }

  // Division/overshift checks and the intrinsic cleaner are KLEE legacy passes.
  {
    legacy::PassManager pm;
    if (CheckDivZero)
      pm.add(new klee::DivCheckPass());
    if (CheckOvershift)
      pm.add(new klee::OvershiftCheckPass());

    // IntrinsicCleanerPass stores the DataLayout by reference, so it must
    // outlive pm.run() below.
    llvm::DataLayout targetData(module->getDataLayout());
    pm.add(new klee::IntrinsicCleanerPass(targetData));
    pm.run(*module);
  }
}

void klee::checkModule(bool DontVerify, llvm::Module *module) {
  klee::InstructionOperandTypeCheckPass *operandTypeCheckPass =
      new klee::InstructionOperandTypeCheckPass();

  legacy::PassManager pm;
  if (!DontVerify)
    pm.add(createVerifierPass());
  pm.add(operandTypeCheckPass);
  pm.run(*module);

  // Enforce the operand type invariants that the Executor expects.  This
  // implicitly depends on the "Scalarizer" pass to be run in order to succeed
  // in the presence of vector instructions.
  if (!operandTypeCheckPass->checkPassed()) {
    klee_error("Unexpected instruction operand types detected");
  }
}
