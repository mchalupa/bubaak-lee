//===-- MemoryManager.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MemoryManager.h"

#include "CoreStats.h"
#include "ExecutionState.h"
#include "Memory.h"

#include "klee/Expr/Expr.h"
#include "klee/Support/ErrorHandling.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/Alignment.h"
DISABLE_WARNING_POP

#include <cinttypes>
#include <algorithm>
#include <sys/mman.h>
#include <tuple>
#include <string>

using namespace klee;

namespace klee {
std::uint32_t MemoryManager::quarantine;

std::size_t MemoryManager::pageSize = sysconf(_SC_PAGE_SIZE);

bool MemoryManager::isDeterministic;

llvm::cl::OptionCategory MemoryCat("Memory management options",
                                   "These options control memory management.");

llvm::cl::opt<bool, true> DeterministicAllocation(
    "kdalloc",
    llvm::cl::desc("Allocate memory deterministically (default=true)"),
    llvm::cl::location(MemoryManager::isDeterministic), llvm::cl::init(true),
    llvm::cl::cat(MemoryCat));

llvm::cl::opt<bool> DeterministicAllocationMarkAsUnneeded(
    "kdalloc-mark-as-unneeded",
    llvm::cl::desc("Mark allocations as unneeded after external function calls "
                   "(default=true)"),
    llvm::cl::init(true), llvm::cl::cat(MemoryCat));

llvm::cl::opt<unsigned> DeterministicAllocationGlobalsSize(
    "kdalloc-globals-size",
    llvm::cl::desc("Reserved memory for globals in GiB (default=10)"),
    llvm::cl::init(10), llvm::cl::cat(MemoryCat));

llvm::cl::opt<unsigned> DeterministicAllocationConstantsSize(
    "kdalloc-constants-size",
    llvm::cl::desc("Reserved memory for constant globals in GiB (default=10)"),
    llvm::cl::init(10), llvm::cl::cat(MemoryCat));

llvm::cl::opt<unsigned> DeterministicAllocationHeapSize(
    "kdalloc-heap-size",
    llvm::cl::desc("Reserved memory for heap in GiB (default=1024)"),
    llvm::cl::init(1024), llvm::cl::cat(MemoryCat));

llvm::cl::opt<unsigned> DeterministicAllocationStackSize(
    "kdalloc-stack-size",
    llvm::cl::desc("Reserved memory for stack in GiB (default=100)"),
    llvm::cl::init(128), llvm::cl::cat(MemoryCat));

llvm::cl::opt<std::uintptr_t> DeterministicAllocationGlobalsStartAddress(
    "kdalloc-globals-start-address",
    llvm::cl::desc(
        "Start address for globals segment (has to be page aligned)"),
    llvm::cl::cat(MemoryCat));

llvm::cl::opt<std::uintptr_t> DeterministicAllocationConstantsStartAddress(
    "kdalloc-constants-start-address",
    llvm::cl::desc(
        "Start address for constant globals segment (has to be page aligned)"),
    llvm::cl::cat(MemoryCat));

llvm::cl::opt<std::uintptr_t> DeterministicAllocationHeapStartAddress(
    "kdalloc-heap-start-address",
    llvm::cl::desc("Start address for heap segment (has to be page aligned)"),
    llvm::cl::cat(MemoryCat));

llvm::cl::opt<std::uintptr_t> DeterministicAllocationStackStartAddress(
    "kdalloc-stack-start-address",
    llvm::cl::desc("Start address for stack segment (has to be page aligned)"),
    llvm::cl::cat(MemoryCat));

struct QuarantineSizeParser : public llvm::cl::parser<std::uint32_t> {
  explicit QuarantineSizeParser(llvm::cl::Option &O)
      : llvm::cl::parser<std::uint32_t>(O) {}

  bool parse(llvm::cl::Option &O, llvm::StringRef ArgName, llvm::StringRef Arg,
             std::uint32_t &Val) {
    if (Arg == "-1") {
      Val = kdalloc::Allocator::unlimitedQuarantine;
    } else if (Arg.getAsInteger(0, Val)) {
      return O.error("'" + Arg + "' value invalid!");
    }

    return false;
  }
};

llvm::cl::opt<std::uint32_t, true, QuarantineSizeParser>
    DeterministicAllocationQuarantineSize(
        "kdalloc-quarantine",
        llvm::cl::desc("Size of quarantine queues in allocator (default=8, "
                       "disabled=0, unlimited=-1)"),
        llvm::cl::location(klee::MemoryManager::quarantine),
        llvm::cl::value_desc("size"), llvm::cl::init(8),
        llvm::cl::cat(MemoryCat));

llvm::cl::opt<bool> NullOnZeroMalloc(
    "return-null-on-zero-malloc",
    llvm::cl::desc("Returns NULL if malloc(0) is called (default=false)"),
    llvm::cl::init(false), llvm::cl::cat(MemoryCat));
} // namespace

/***/
MemoryManager::MemoryManager(ArrayCache *_arrayCache)
    : arrayCache(_arrayCache), lastSegment(0) {
  if (DeterministicAllocation) {
    if (DeterministicAllocationQuarantineSize ==
        kdalloc::Allocator::unlimitedQuarantine) {
      klee_message("Deterministic allocator: Using unlimited quarantine");
    } else if (DeterministicAllocationQuarantineSize != 0) {
      klee_message("Deterministic allocator: Using quarantine queue size %u",
                   DeterministicAllocationQuarantineSize.getValue());
    }

    std::vector<std::tuple<std::string,
                           std::uintptr_t, // start address (0 if none
                                           // requested)
                           std::size_t,    // size of segment
                           std::reference_wrapper<kdalloc::AllocatorFactory>,
                           kdalloc::Allocator *>>
        requestedSegments;
    requestedSegments.emplace_back(
        "globals",
        DeterministicAllocationGlobalsStartAddress
            ? DeterministicAllocationGlobalsStartAddress.getValue()
            : 0,
        static_cast<std::size_t>(
            DeterministicAllocationGlobalsSize.getValue()) *
            1024 * 1024 * 1024,
        globalsFactory, &globalsAllocator);
    requestedSegments.emplace_back(
        "constants",
        DeterministicAllocationConstantsStartAddress
            ? DeterministicAllocationConstantsStartAddress.getValue()
            : 0,
        static_cast<std::size_t>(
            DeterministicAllocationConstantsSize.getValue()) *
            1024 * 1024 * 1024,
        constantsFactory, &constantsAllocator);
    requestedSegments.emplace_back(
        "heap",
        DeterministicAllocationHeapStartAddress
            ? DeterministicAllocationHeapStartAddress.getValue()
            : 0,
        static_cast<std::size_t>(DeterministicAllocationHeapSize.getValue()) *
            1024 * 1024 * 1024,
        heapFactory, nullptr);
    requestedSegments.emplace_back(
        "stack",
        DeterministicAllocationStackStartAddress
            ? DeterministicAllocationStackStartAddress.getValue()
            : 0,
        static_cast<std::size_t>(DeterministicAllocationStackSize.getValue()) *
            1024 * 1024 * 1024,
        stackFactory, nullptr);

    // check invariants
    llvm::Align pageAlignment(pageSize);
    for (auto &requestedSegment : requestedSegments) {
      auto &segment1 = std::get<0>(requestedSegment);
      auto &start1 = std::get<1>(requestedSegment);
      auto &size1 = std::get<2>(requestedSegment);
      // check for page alignment
      // NOTE: sizes are assumed to be page aligned due to multiplication
      if (start1 != 0 && !llvm::isAligned(pageAlignment, start1)) {
        klee_error("Deterministic allocator: Requested start address for %s "
                   "is not page aligned (page size: %" PRIu64 " B)",
                   segment1.c_str(), pageAlignment.value());
      }

      // check for overlap of segments
      std::uintptr_t end1 = start1 + size1;
      for (auto &requestedSegment : requestedSegments) {
        auto &segment2 = std::get<0>(requestedSegment);
        auto &start2 = std::get<1>(requestedSegment);
        auto &size2 = std::get<2>(requestedSegment);
        if (start1 != 0 && start2 != 0 && segment1 != segment2) {
          std::uintptr_t end2 = start2 + size2;
          if (!(end1 <= start2 || start1 >= end2)) {
            klee_error("Deterministic allocator: Requested mapping for %s "
                       "(start-address=0x%" PRIxPTR " size=%zu GiB) "
                       "overlaps with that for %s "
                       "(start-address=0x%" PRIxPTR " size=%zu GiB)",
                       segment1.c_str(), start1, size1 / (1024 * 1024 * 1024),
                       segment2.c_str(), start2, size2 / (1024 * 1024 * 1024));
          }
        }
      }
    }

    // initialize factories and allocators
    for (auto &requestedSegment : requestedSegments) {
      auto &segment = std::get<0>(requestedSegment);
      auto &start = std::get<1>(requestedSegment);
      auto &size = std::get<2>(requestedSegment);
      auto &factory = std::get<3>(requestedSegment);
      auto &allocator = std::get<4>(requestedSegment);
      factory.get() = kdalloc::AllocatorFactory(
          start, size, DeterministicAllocationQuarantineSize);

      if (!factory.get()) {
        klee_error("Deterministic allocator: Could not allocate mapping for %s "
                   "(start-address=0x%" PRIxPTR " size=%zu GiB): %s",
                   segment.c_str(), start, size / (1024 * 1024 * 1024),
                   strerror(errno));
      }
      if (start && factory.get().getMapping().getBaseAddress() !=
                       reinterpret_cast<void *>(start)) {
        klee_error("Deterministic allocator: Could not allocate mapping for %s "
                   "at requested address",
                   segment.c_str());
      }
      if (factory.get().getMapping().getSize() != size) {
        klee_error("Deterministic allocator: Could not allocate mapping for %s "
                   "with the requested size",
                   segment.c_str());
      }

      klee_message("Deterministic allocator: %s "
                   "(start-address=0x%" PRIxPTR " size=%zu GiB)",
                   segment.c_str(),
                   reinterpret_cast<std::uintptr_t>(
                       factory.get().getMapping().getBaseAddress()),
                   size / (1024 * 1024 * 1024));
      if (allocator) {
        *allocator = factory.get().makeAllocator();
      }
    }
  }
}

MemoryManager::~MemoryManager() {
  while (!objects.empty()) {
    MemoryObject *mo = *objects.begin();
    if (!mo->isFixed && !DeterministicAllocation)
      free((void *)mo->address);
    objects.erase(mo);
    delete mo;
  }
}

MemoryObject *MemoryManager::allocate(uint64_t size, bool isLocal,
                                      bool isGlobal, ExecutionState *state,
                                      const llvm::Value *allocSite,
                                      size_t alignment) {
  ref<Expr> sizeExpr = ConstantExpr::alloc(size, Context::get().getPointerWidth());
  return allocate(sizeExpr, isLocal, isGlobal, state, allocSite, alignment);
}

MemoryObject *MemoryManager::allocate(ref<Expr> size, bool isLocal,
                                      bool isGlobal, ExecutionState *state,
                                      const llvm::Value *allocSite,
                                      size_t alignment) {
  uint64_t concreteSize = 0;
  bool hasConcreteSize = false;
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(size)) {
    hasConcreteSize = true;
    concreteSize = CE->getZExtValue();
  }

  if (concreteSize > 10 * 1024 * 1024) {
    klee_warning_once(nullptr,
                      "Large memory allocation (%" PRIu64 " bytes). "
                      "KLEE may run out of memory.",
                      concreteSize);
    hasConcreteSize = false;
  }

  // Return NULL if size is zero, this is equal to error during allocation
  if (NullOnZeroMalloc && hasConcreteSize && concreteSize == 0)
    return 0;

  if (!llvm::isPowerOf2_64(alignment)) {
    klee_warning("Only alignment of power of two is supported");
    return 0;
  }

  uint64_t address = 0;
  if (DeterministicAllocation) {
    void *allocAddress;

    size_t kdalloc_size = std::max(hasConcreteSize ? concreteSize : (uint64_t)1,
                                   (uint64_t)alignment);
    if (isGlobal) {
      const llvm::GlobalVariable *gv =
          dyn_cast<llvm::GlobalVariable>(allocSite);
      if (isa<llvm::Function>(allocSite) || (gv && gv->isConstant())) {
        allocAddress = constantsAllocator.allocate(kdalloc_size);
      } else {
        allocAddress = globalsAllocator.allocate(kdalloc_size);
      }
    } else {
      if (isLocal) {
        allocAddress = state->stackAllocator.allocate(kdalloc_size);
      } else {
        allocAddress = state->heapAllocator.allocate(kdalloc_size);
      }
    }

    address = reinterpret_cast<std::uint64_t>(allocAddress);
  } else {
    // allocate 1 byte for symbolic-size allocation, just so we get an address
    size_t alloc_size = hasConcreteSize ? concreteSize : 1;
    // Use malloc for the standard case
    if (alignment <= 8)
      address = (uint64_t)malloc(alloc_size);
    else {
      int res = posix_memalign((void **)&address, alignment, alloc_size);
      if (res < 0) {
        klee_warning("Allocating aligned memory failed.");
        address = 0;
      }
    }
  }

  if (!address)
    return 0;

  ++stats::allocations;
  MemoryObject *res = new MemoryObject(++lastSegment, (uint64_t)address,
                                       size, concreteSize,
                                       isLocal, isGlobal, false, allocSite, this);
  objects.insert(res);
  return res;
}

MemoryObject *MemoryManager::allocateFixed(uint64_t address, uint64_t size,
                                           const llvm::Value *allocSite) {
#ifndef NDEBUG
  for (objects_ty::iterator it = objects.begin(), ie = objects.end(); it != ie;
       ++it) {
    MemoryObject *mo = *it;
    // symbolic size objects can overlap
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(mo->size)) {
      unsigned moSize = CE->getZExtValue();
      if (address + moSize > mo->address && address < mo->address + moSize)
        klee_error("Trying to allocate an overlapping object");
    }
  }
#endif

  ++stats::allocations;
  ref<Expr> sizeExpr = ConstantExpr::alloc(size, Context::get().getPointerWidth());
  MemoryObject *res =
      new MemoryObject(++lastSegment, address, sizeExpr, size, false, true, true, allocSite, this);
  objects.insert(res);
  return res;
}

void MemoryManager::markFreed(MemoryObject *mo) {
  if (objects.find(mo) != objects.end()) {
    if (!mo->isFixed && !DeterministicAllocation)
      free((void *)mo->address);
    objects.erase(mo);
  }
}

bool MemoryManager::markMappingsAsUnneeded() {
  if (!DeterministicAllocation)
    return false;

  if (!DeterministicAllocationMarkAsUnneeded)
    return false;

  globalsFactory.getMapping().clear();
  heapFactory.getMapping().clear();
  stackFactory.getMapping().clear();

  return true;
}

size_t MemoryManager::getUsedDeterministicSize() const {
  // TODO: implement
  return 0;
}
