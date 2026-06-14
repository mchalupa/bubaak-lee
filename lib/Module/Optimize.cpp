//===- Optimize.cpp - Optimize a complete program -------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// New-PassManager based implementation of the module optimization and
// preparation passes used for LLVM >= 17.  The built-in optimization pipeline
// is built through PassBuilder, while KLEE's own (legacy) passes are still run
// through a legacy::PassManager.
//
//===----------------------------------------------------------------------===//

#include "ModuleHelper.h"

#include "Passes.h"
#include "klee/Support/CompilerWarning.h"
#include "klee/Support/OptionCategories.h"

DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO/GlobalDCE.h"
#include "llvm/Transforms/IPO/Internalize.h"
#include "llvm/Transforms/IPO/StripSymbols.h"
#include "llvm/Transforms/Scalar/Scalarizer.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils/LowerSwitch.h"
DISABLE_WARNING_POP

#include <set>
#include <string>

using namespace llvm;
using namespace klee;

namespace {
cl::opt<bool>
    DisableInline("disable-inlining",
                  cl::desc("Do not run the inliner pass (default=false)"),
                  cl::init(false), cl::cat(klee::ModuleCat));

cl::opt<bool> DisableInternalize(
    "disable-internalize",
    cl::desc("Do not mark all symbols as internal (default=false)"),
    cl::init(false), cl::cat(klee::ModuleCat));

cl::opt<bool> Strip("strip-all",
                    cl::desc("Strip all symbol information from executable"),
                    cl::init(false), cl::cat(klee::ModuleCat));

cl::opt<bool>
    StripDebug("strip-debug",
               cl::desc("Strip debugger symbol info from executable"),
               cl::init(false), cl::cat(klee::ModuleCat));

// Helper that wires up the analysis managers required by the new PassManager
// and runs the given module pass manager over the module.
struct NewPMRunner {
  PassBuilder PB;
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;

  NewPMRunner(llvm::Module &M) {
    // Prevent the optimizer from synthesising library calls that KLEE's
    // freestanding runtime does not provide. In particular the pipeline would
    // otherwise turn "memcmp(...) == 0" into bcmp, which is then introduced
    // after the (lazily linked) runtime has been linked and ends up as an
    // unresolved external call. Registering a TargetLibraryAnalysis with bcmp
    // marked unavailable must happen before registerFunctionAnalyses so it
    // takes precedence over the default one.
    llvm::TargetLibraryInfoImpl TLII(llvm::Triple(M.getTargetTriple()));
    TLII.setUnavailable(llvm::LibFunc_bcmp);
    FAM.registerPass([TLII] { return llvm::TargetLibraryAnalysis(TLII); });

    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
  }

  void run(ModulePassManager &MPM, llvm::Module &M) { MPM.run(M, MAM); }
};
} // namespace

void klee::optimizeModule(llvm::Module *M,
                          llvm::ArrayRef<const char *> preservedFunctions) {
  NewPMRunner runner(*M);
  ModulePassManager MPM;

  // Mark all symbols other than the preserved ones as internal so that the
  // optimizer can specialise/remove them.
  if (!DisableInternalize) {
    std::set<std::string> preserve;
    for (const char *fun : preservedFunctions)
      preserve.insert(fun);

    MPM.addPass(InternalizePass([preserve](const GlobalValue &GV) {
      return preserve.count(GV.getName().str()) != 0;
    }));
    MPM.addPass(GlobalDCEPass());
  }

  // Run the standard per-module optimization pipeline.  The exact set of
  // passes differs from the hand-tuned legacy pipeline, but it performs all
  // the simplifications KLEE relies on (mem2reg, inlining, CFG cleanup, etc.).
  OptimizationLevel level =
      DisableInline ? OptimizationLevel::O1 : OptimizationLevel::O2;
  MPM.addPass(runner.PB.buildPerModuleDefaultPipeline(level));

  if (Strip || StripDebug)
    MPM.addPass(StripSymbolsPass());

  runner.run(MPM, *M);
}

void klee::optimiseAndPrepare(bool OptimiseKLEECall, bool Optimize,
                              SwitchImplType SwitchType, std::string EntryPoint,
                              llvm::ArrayRef<const char *> preservedFunctions,
                              llvm::Module *module) {
  // Preserve all functions containing klee-related function calls from being
  // optimised around.
  if (!OptimiseKLEECall) {
    legacy::PassManager pm;
    pm.add(new klee::OptNonePass());
    pm.run(*module);
  }

  if (Optimize)
    optimizeModule(module, preservedFunctions);

  // Needs to happen after linking (since ctors/dtors can be modified)
  // and optimization (since global optimization can rewrite lists).
  injectStaticConstructorsAndDestructors(module, EntryPoint);

  // Finally, run the passes that maintain invariants we expect during
  // interpretation.  CFG simplification and (optionally) the LLVM switch
  // lowering are built-in passes run through the new PassManager.
  {
    NewPMRunner runner(*module);
    FunctionPassManager FPM;
    FPM.addPass(SimplifyCFGPass());
    if (SwitchType == SwitchImplType::eSwitchTypeLLVM)
      FPM.addPass(llvm::LowerSwitchPass());

    ModulePassManager MPM;
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
    runner.run(MPM, *module);
  }

  // KLEE's switch lowering and the intrinsic cleaner are legacy passes.
  {
    legacy::PassManager pm;
    if (SwitchType == SwitchImplType::eSwitchTypeSimple)
      pm.add(new klee::LowerSwitchPass());

    // IntrinsicCleanerPass stores the DataLayout by reference, so it must
    // outlive pm.run() below.
    llvm::DataLayout targetData(module->getDataLayout());
    pm.add(new klee::IntrinsicCleanerPass(targetData));
    pm.run(*module);
  }

  // Scalarizer is a built-in transform (new PassManager).
  {
    NewPMRunner runner(*module);
    FunctionPassManager FPM;
    FPM.addPass(ScalarizerPass());

    ModulePassManager MPM;
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
    runner.run(MPM, *module);
  }

  // PhiCleaner and FunctionAlias are KLEE legacy passes.
  {
    legacy::PassManager pm;
    pm.add(new klee::PhiCleanerPass());
    pm.add(new klee::FunctionAliasPass());
    pm.run(*module);
  }
}
