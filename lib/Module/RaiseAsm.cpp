//===-- RaiseAsm.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Passes.h"
#include "klee/Config/Version.h"
#include "klee/Support/ErrorHandling.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(16, 0)
#include "llvm/TargetParser/Host.h"
#else
#include "llvm/Support/Host.h"
#endif
#if LLVM_VERSION_CODE >= LLVM_VERSION(14, 0)
#include "llvm/MC/TargetRegistry.h"
#else
#include "llvm/Support/TargetRegistry.h"
#endif
#include "llvm/Target/TargetMachine.h"
DISABLE_WARNING_POP

using namespace llvm;
using namespace klee;

char RaiseAsmPass::ID = 0;

Function *RaiseAsmPass::getIntrinsic(llvm::Module &M, unsigned IID, Type **Tys,
                                     unsigned NumTys) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(20, 0)
  return Intrinsic::getOrInsertDeclaration(
      &M, (llvm::Intrinsic::ID)IID, llvm::ArrayRef<llvm::Type *>(Tys, NumTys));
#else
  return Intrinsic::getDeclaration(&M, (llvm::Intrinsic::ID) IID,
                                   llvm::ArrayRef<llvm::Type*>(Tys, NumTys));
#endif
}

// Lower an inline assembly with an empty string: it computes nothing, so an
// output that is tied to an input (e.g., "+r"(x), which is "=r,0") keeps the
// value of that input, and the indirect outputs (memory) keep their contents.
// Returns false (and leaves the call untouched) if some direct output is not
// tied to an input: its value is unknown.
static bool lowerEmptyAsm(CallInst *ci, InlineAsm *ia) {
  const auto constraints = ia->ParseConstraints();

  // the index of the call argument of every constraint that takes one
  // (inputs and indirect outputs), -1 for the other constraints
  std::vector<int> argIndex(constraints.size(), -1);
  unsigned args = 0;
  for (unsigned i = 0; i < constraints.size(); ++i) {
    const auto &constr = constraints[i];
    if (constr.Type == InlineAsm::isInput ||
        (constr.Type == InlineAsm::isOutput && constr.isIndirect))
      argIndex[i] = args++;
  }
  if (args != ci->arg_size())
    return false;

  // the values of the direct outputs (the results of the call), in order
  std::vector<Value *> outputs;
  for (const auto &constr : constraints) {
    if (constr.Type != InlineAsm::isOutput || constr.isIndirect)
      continue;
    // MatchingInput is the index of the tied input among the constraints
    if (!constr.hasMatchingInput() ||
        static_cast<unsigned>(constr.MatchingInput) >= constraints.size() ||
        argIndex[constr.MatchingInput] < 0)
      return false;
    outputs.push_back(ci->getArgOperand(argIndex[constr.MatchingInput]));
  }

  Type *retTy = ci->getType();
  StructType *structTy = dyn_cast<StructType>(retTy);
  if (retTy->isVoidTy()) {
    if (!outputs.empty())
      return false;
  } else if (outputs.size() == 1 && outputs[0]->getType() == retTy) {
    structTy = nullptr;
  } else {
    // several outputs are returned in a struct
    if (!structTy || structTy->getNumElements() != outputs.size())
      return false;
    for (unsigned i = 0; i < outputs.size(); ++i) {
      if (outputs[i]->getType() != structTy->getElementType(i))
        return false;
    }
  }

  IRBuilder<> Builder(ci);
  if (ia->hasSideEffects())
    Builder.CreateFence(llvm::AtomicOrdering::SequentiallyConsistent);
  if (!retTy->isVoidTy()) {
    Value *result = outputs[0];
    if (structTy) {
      result = UndefValue::get(structTy);
      for (unsigned i = 0; i < outputs.size(); ++i)
        result = Builder.CreateInsertValue(result, outputs[i], i);
    }
    ci->replaceAllUsesWith(result);
  }
  ci->eraseFromParent();
  return true;
}

// FIXME: This should just be implemented as a patch to
// X86TargetAsmInfo.cpp, then everyone will benefit.
bool RaiseAsmPass::runOnInstruction(Module &M, Instruction *I) {
  // We can just raise inline assembler using calls
  CallInst *ci = dyn_cast<CallInst>(I);
  if (!ci)
    return false;

  InlineAsm *ia = dyn_cast<InlineAsm>(ci->getCalledOperand());
  if (!ia)
    return false;

  // Try to use existing infrastructure
  if (!TLI)
    return false;

#if LLVM_VERSION_CODE < LLVM_VERSION(22, 0)
  // TargetLowering::ExpandInlineAsm was removed in LLVM 22. When it is not
  // available we simply fall through to the manual handling below.
  if (TLI->ExpandInlineAsm(ci))
    return true;
#endif

  if ((triple.getArch() == llvm::Triple::x86 ||
       triple.getArch() == llvm::Triple::x86_64) &&
      (triple.isOSLinux() || triple.isMacOSX() || triple.isOSFreeBSD())) {

    if (ia->getAsmString().empty() && lowerEmptyAsm(ci, ia))
      return true;
  }

  return false;
}

bool RaiseAsmPass::runOnModule(Module &M) {
  bool changed = false;
  std::string Err;

  // Use target triple from the module if possible.
#if LLVM_VERSION_CODE >= LLVM_VERSION(21, 0)
  // Module::getTargetTriple() returns a llvm::Triple by reference, and the
  // target-machine / lookup APIs now take a Triple directly.
  llvm::Triple TargetTriple = M.getTargetTriple();
  if (TargetTriple.getTriple().empty())
    TargetTriple = llvm::Triple(llvm::sys::getDefaultTargetTriple());
  const Target *Target = TargetRegistry::lookupTarget(TargetTriple, Err);

  TargetMachine *TM = 0;
  if (Target == 0) {
    klee_warning("Warning: unable to select target: %s", Err.c_str());
    TLI = 0;
  } else {
    TM = Target->createTargetMachine(TargetTriple, "", "", TargetOptions(),
                                     std::nullopt);

    TLI = TM->getSubtargetImpl(*(M.begin()))->getTargetLowering();

    triple = TargetTriple;
  }
#else
  std::string TargetTriple = M.getTargetTriple();
  if (TargetTriple.empty())
    TargetTriple = llvm::sys::getDefaultTargetTriple();
  const Target *Target = TargetRegistry::lookupTarget(TargetTriple, Err);

  TargetMachine * TM = 0;
  if (Target == 0) {
    klee_warning("Warning: unable to select target: %s", Err.c_str());
    TLI = 0;
  } else {
#if LLVM_VERSION_CODE >= LLVM_VERSION(16, 0)
    TM = Target->createTargetMachine(TargetTriple, "", "", TargetOptions(),
                                     std::nullopt);
#else
    TM = Target->createTargetMachine(TargetTriple, "", "", TargetOptions(),
                                     None);
#endif

    TLI = TM->getSubtargetImpl(*(M.begin()))->getTargetLowering();

    triple = llvm::Triple(TargetTriple);
  }
#endif

  for (Module::iterator fi = M.begin(), fe = M.end(); fi != fe; ++fi) {
    for (Function::iterator bi = fi->begin(), be = fi->end(); bi != be; ++bi) {
      for (BasicBlock::iterator ii = bi->begin(), ie = bi->end(); ii != ie;) {
        Instruction *i = &*ii;
        ++ii;  
        changed |= runOnInstruction(M, i);
      }
    }
  }

  delete TM;

  return changed;
}
