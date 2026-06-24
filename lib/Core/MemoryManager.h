//===-- MemoryManager.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_MEMORYMANAGER_H
#define KLEE_MEMORYMANAGER_H

#include "klee/Expr/Expr.h"

#include <cstddef>
#include <cstdint>
#include <set>
#include <unordered_map>
#include <vector>

namespace llvm {
class Value;
}

namespace klee {
class ArrayCache;
class ExecutionState;
class MemoryObject;

/// A single mmap'd region from which memory is handed out by bumping a pointer.
class MmapAllocation {
  void *data{nullptr};
  void *nextFreeSlot{nullptr};
  size_t dataSize{0};
  void *expectedAddress{nullptr};
  int flags{0};

  void *getNextFreeSlot(size_t alignment) const;
  void initialize();

public:
  MmapAllocation() = default;
  MmapAllocation(size_t spacesize, void *expectedAddr = nullptr, int flags = 0);
  MmapAllocation(MmapAllocation &&);
  MmapAllocation(const MmapAllocation &) = delete;
  ~MmapAllocation();

  void initialize(size_t datasize, void *expectedAddr = nullptr, int flags = 0);
  bool hasSpace(size_t size, size_t alignment) const;
  void *allocate(size_t size, size_t alignment);
  size_t getUsedSize() const;
};

/// A growing collection of mmap'd blocks of a fixed block size.
class MmapAllocator {
  size_t blockSize{4096};
  int flags{0};
  std::vector<MmapAllocation> blocks;

public:
  MmapAllocator(size_t blksize = 4096, int flags = 0)
      : blockSize(blksize), flags(flags) {}

  void *allocate(size_t size, size_t alignment);
  size_t getUsedSize() const;
};

/// Routes an allocation to an MmapAllocator with a suitable block size.
class AllocatorMap {
  const size_t BASE_BLOCK_SIZE = 4096;

  int flags{0};
  std::unordered_map<size_t, MmapAllocator> allocators;

  size_t getAppropriateBlockSize(size_t allocationSize) const;
  MmapAllocator &getOrCreateAllocator(size_t blockSize);

public:
  AllocatorMap(int flags = 0) : flags(flags) {}
  void *allocate(size_t size, size_t alignment);
};

/// Runtime-selectable allocator object used to obtain real process memory.
/// The default is plain malloc; deterministic (a single mmap'd bump region) and
/// 32-bit/low-memory modes are selectable via command-line options.
class MemoryAllocator {
  bool deterministic{false};
  // allocate memory in the lower 32-bit memory space
  bool lowmem{false};

  MmapAllocation deterministicMem{};
  AllocatorMap lowmemAllocator;

public:
  MemoryAllocator(bool determ, bool lowmem, size_t determ_size,
                  void *expectedAddr);

  void *allocate(size_t size, size_t alignment);
  void deallocate(void *);
  void useLowMemory(bool lm);

  size_t getUsedDeterministicSize() const {
    return deterministicMem.getUsedSize();
  }
};

class MemoryManager {
private:
  typedef std::set<MemoryObject *> objects_ty;
  objects_ty objects;
  ArrayCache *const arrayCache;

  MemoryAllocator allocator;
  uint64_t lastSegment;

public:
  explicit MemoryManager(ArrayCache *arrayCache);
  ~MemoryManager();

  /// Create a MemoryObject. Object addresses are symbolic in this memory model,
  /// so this does NOT allocate any real process memory; the object is
  /// identified by its segment and records the size that an external call would
  /// need as allocatedSize.
  MemoryObject *allocate(uint64_t size, bool isLocal, bool isGlobal,
                         ExecutionState *state, const llvm::Value *allocSite,
                         size_t alignment);
  MemoryObject *allocate(ref<Expr> size, bool isLocal, bool isGlobal,
                         ExecutionState *state, const llvm::Value *allocSite,
                         size_t alignment);
  MemoryObject *allocateFixed(uint64_t size, const llvm::Value *allocSite,
                              uint64_t specialSegment = 0);
  void deallocate(const MemoryObject *mo);
  void markFreed(MemoryObject *mo);
  ArrayCache *getArrayCache() const { return arrayCache; }

  uint64_t getLastSegment() const { return lastSegment; }

  /// Returns the size used by deterministic allocation in bytes.
  size_t getUsedDeterministicSize() const;

  void useLowMemory(bool lm) { allocator.useLowMemory(lm); }

  /// Allocate real process memory transiently, only for the duration of an
  /// external function call (see Executor::callExternalFunction). The returned
  /// memory must be released with deallocateMemory().
  void *allocateMemory(size_t size, size_t alignment) {
    return allocator.allocate(size, alignment);
  }

  /// Release memory obtained from allocateMemory().
  void deallocateMemory(void *ptr) { allocator.deallocate(ptr); }
};

} // End klee namespace

#endif /* KLEE_MEMORYMANAGER_H */
