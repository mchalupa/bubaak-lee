//===-- Memory.cpp --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Memory.h"

#include "Context.h"
#include "ExecutionState.h"
#include "Executor.h"
#include "MemoryManager.h"

#include "klee/Expr/ArrayCache.h"
#include "klee/Expr/Expr.h"
#include "klee/Support/OptionCategories.h"
#include "klee/Solver/Solver.h"
#include "klee/Support/ErrorHandling.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include <cassert>
#include <sstream>

using namespace llvm;
using namespace klee;

namespace {
  cl::opt<bool>
  UseConstantArrays("use-constant-arrays",
                    cl::desc("Use constant arrays instead of updates when possible (default=true)\n"),
                    cl::init(true),
                    cl::cat(SolvingCat));
}

/***/

int MemoryObject::counter = 0;

MemoryObject::~MemoryObject() {
  if (parent)
    parent->markFreed(this);
}

void MemoryObject::getAllocInfo(std::string &result) const {
  llvm::raw_string_ostream info(result);

  info << "MO" << id << "[" << getSizeString() << "]";

  if (allocSite) {
    info << " allocated at ";
    if (const Instruction *i = dyn_cast<Instruction>(allocSite)) {
      info << i->getParent()->getParent()->getName() << "():";
      info << *i;
    } else if (const GlobalValue *gv = dyn_cast<GlobalValue>(allocSite)) {
      info << "global:" << gv->getName();
    } else {
      info << "value:" << *allocSite;
    }
  } else {
    info << " (no allocation info)";
  }
  
  info.flush();
}

/***/

ObjectStatePlane::ObjectStatePlane(const ObjectState *parent)
  : parent(parent),
    updates(nullptr, nullptr),
    sizeBound(0),
    symbolic(false),
    initialValue(0) {
  if (!UseConstantArrays) {
    static unsigned id = 0;
    const Array *array =
        parent->getArrayCache()->CreateArray("tmp_arr" + llvm::utostr(++id), sizeBound);
    updates = UpdateList(array, 0);
  }
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(parent->getObject()->size)) {
    sizeBound = CE->getZExtValue();
  }
}


ObjectStatePlane::ObjectStatePlane(const ObjectState *parent, const Array *array)
  : parent(parent),
    updates(array, nullptr),
    sizeBound(0),
    symbolic(true),
    initialValue(0) {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(parent->getObject()->size)) {
    sizeBound = CE->getZExtValue();
  }
}

ObjectStatePlane::ObjectStatePlane(const ObjectState *parent, const ObjectStatePlane &os)
  : parent(parent),
    concreteStore(os.concreteStore),
    concreteMask(os.concreteMask),
    knownSymbolics(os.knownSymbolics),
    unflushedMask(os.unflushedMask),
    updates(os.updates),
    sizeBound(os.sizeBound),
    symbolic(os.symbolic),
    initialValue(os.initialValue) {
  assert(!os.parent->readOnly && "no need to copy read only object?");
}

/***/

const UpdateList &ObjectStatePlane::getUpdates() const {
  // Constant arrays are created lazily.
  if (!updates.root) {
    // Collect the list of writes, with the oldest writes first.
    
    // FIXME: We should be able to do this more efficiently, we just need to be
    // careful to get the interaction with the cache right. In particular we
    // should avoid creating UpdateNode instances we never use.
    unsigned NumWrites = updates.head ? updates.head->getSize() : 0;
    std::vector< std::pair< ref<Expr>, ref<Expr> > > Writes(NumWrites);
    const auto *un = updates.head.get();
    for (unsigned i = NumWrites; i != 0; un = un->next.get()) {
      --i;
      Writes[i] = std::make_pair(un->index, un->value);
    }

    std::vector< ref<ConstantExpr> > Contents(sizeBound);

    // Initialize to zeros.
    for (unsigned i = 0, e = sizeBound; i != e; ++i)
      Contents[i] = ConstantExpr::create(0, Expr::Int8);

    // Pull off as many concrete writes as we can.
    unsigned Begin = 0, End = Writes.size();
    for (; Begin != End; ++Begin) {
      // Push concrete writes into the constant array.
      ConstantExpr *Index = dyn_cast<ConstantExpr>(Writes[Begin].first);
      if (!Index)
        break;

      ConstantExpr *Value = dyn_cast<ConstantExpr>(Writes[Begin].second);
      if (!Value)
        break;

      Contents[Index->getZExtValue()] = Value;
    }

    static unsigned id = 0;
    const Array *array;
    if (sizeBound == 0) {
       array = parent->getArrayCache()->CreateArray(
          "const_arr" + llvm::utostr(++id), sizeBound);
    } else {
      array = parent->getArrayCache()->CreateArray(
          "const_arr" + llvm::utostr(++id), sizeBound, &Contents[0],
          &Contents[0] + Contents.size());
    }
    updates = UpdateList(array, 0);

    // Apply the remaining (non-constant) writes.
    for (; Begin != End; ++Begin)
      updates.extend(Writes[Begin].first, Writes[Begin].second);
  }

  return updates;
}

void ObjectStatePlane::flushToConcreteStore(Executor &executor,
                                       ExecutionState &state, bool concretize) {
  for (size_t i = 0; i < sizeBound; i++) {
    if (isByteConcrete(i))
      continue;
    ref<ConstantExpr> ce =
        executor.toConstant(state, read8(i), "external call", concretize);
    if (concreteStore.size() <= i)
      concreteStore.resize(sizeBound);
    uint8_t value;
    ce->toMemory(&value);
    concreteStore[i] = value;
  }
}

void ObjectStatePlane::makeConcrete() {
  concreteMask.resize(0);
  unflushedMask.resize(0);
  knownSymbolics.resize(0);
}

void ObjectStatePlane::makeSymbolic() {
  assert(!updates.head &&
         "XXX makeSymbolic of objects with symbolic values is unsupported");

  for (unsigned i = 0; i < sizeBound; i++) {
    markByteSymbolic(i);
    setKnownSymbolic(i, 0);
    markByteFlushed(i);
  }
}

void ObjectStatePlane::initializeToZero() {
  makeConcrete();
  initialValue = 0;
}

void ObjectStatePlane::initializeToRandom() {
  makeConcrete();
  // randomly selected by 256 sided die
  initialValue = 0xAB;
}

/*
Cache Invariants
--
isByteKnownSymbolic(i) => !isByteConcrete(i)
isByteConcrete(i) => !isByteKnownSymbolic(i)
isByteUnflushed(i) => (isByteConcrete(i) || isByteKnownSymbolic(i))
 */

void ObjectStatePlane::flushForRead() const {
  for (unsigned offset = 0; offset < sizeBound; offset++) {
    if (isByteUnflushed(offset)) {
      if (isByteConcrete(offset)) {
        updates.extend(ConstantExpr::create(offset, Expr::Int32),
                       ConstantExpr::create(getConcreteValue(offset), Expr::Int8));
      } else {
        assert(isByteKnownSymbolic(offset) &&
               "invalid bit set in unflushedMask");
        updates.extend(ConstantExpr::create(offset, Expr::Int32),
                       knownSymbolics[offset]);
      }

      markByteFlushed(offset);
    }
  }
}

void ObjectStatePlane::flushForWrite() {
  for (unsigned offset = 0; offset < sizeBound; offset++) {
    if (isByteUnflushed(offset)) {
      if (isByteConcrete(offset)) {
        updates.extend(ConstantExpr::create(offset, Expr::Int32),
                       ConstantExpr::create(getConcreteValue(offset), Expr::Int8));
        markByteSymbolic(offset);
      } else {
        assert(isByteKnownSymbolic(offset) &&
               "invalid bit set in unflushedMask");
        updates.extend(ConstantExpr::create(offset, Expr::Int32),
                       knownSymbolics[offset]);
        setKnownSymbolic(offset, 0);
      }

      markByteFlushed(offset);
    } else {
      // flushed bytes that are written over still need to be marked out
      markByteSymbolic(offset);
      setKnownSymbolic(offset, 0);
    }
  }
  // everything is potentially overwritten
  symbolic = true;
}

bool ObjectStatePlane::isByteConcrete(size_t offset) const {
  if (offset < concreteMask.size())
    return concreteMask.get(offset);
  return !symbolic;
}

bool ObjectStatePlane::isByteUnflushed(size_t offset) const {
  if (offset < unflushedMask.size())
    return unflushedMask.get(offset);
  return !symbolic;
}

bool ObjectStatePlane::isByteKnownSymbolic(size_t offset) const {
  return offset < knownSymbolics.size() && knownSymbolics[offset].get();
}

void ObjectStatePlane::markByteConcrete(size_t offset) {
  if (offset >= concreteMask.size()) {
    if (!symbolic)
      return;
    concreteMask.resize(sizeBound, !symbolic);
  }
  concreteMask.set(offset);
}

void ObjectStatePlane::markByteSymbolic(size_t offset) {
  if (offset >= concreteMask.size()) {
    if (symbolic)
      return;
    concreteMask.resize(sizeBound, !symbolic);
  }
  concreteMask.unset(offset);
}

void ObjectStatePlane::markByteUnflushed(size_t offset) const {
  if (offset >= unflushedMask.size()) {
    if (!symbolic)
      return;
    unflushedMask.resize(sizeBound, !symbolic);
  }
  unflushedMask.set(offset);
}

void ObjectStatePlane::markByteFlushed(size_t offset) const {
  if (offset >= unflushedMask.size()) {
    if (symbolic)
      return;
    unflushedMask.resize(sizeBound, !symbolic);
  }
  unflushedMask.unset(offset);
}

void ObjectStatePlane::setKnownSymbolic(size_t offset,
                                        Expr *value /* can be null */) {
  if (knownSymbolics.size() <= offset) {
    if (!value)
      return;
    knownSymbolics.resize(sizeBound);
  }
  knownSymbolics[offset] = value;
}

uint8_t ObjectStatePlane::getConcreteValue(unsigned offset) const {
  if (offset < concreteStore.size())
    return concreteStore[offset];
  return initialValue;
}

/***/

ref<Expr> ObjectStatePlane::read8(unsigned offset) const {
  if (isByteConcrete(offset)) {
    return ConstantExpr::create(getConcreteValue(offset), Expr::Int8);
  } else if (isByteKnownSymbolic(offset)) {
    return knownSymbolics[offset];
  } else {
    assert(!isByteUnflushed(offset) && "unflushed byte without cache value");
    
    return ReadExpr::create(getUpdates(), 
                            ConstantExpr::create(offset, Expr::Int32));
  }    
}

ref<Expr> ObjectStatePlane::read8(Executor &executor, ExecutionState &state, ref<Expr> offset) const {
  assert(!isa<ConstantExpr>(offset) &&
         "constant offset passed to symbolic read8");
  flushForRead();

  if (sizeBound > 4096) {
    std::string allocInfo;
    parent->getObject()->getAllocInfo(allocInfo);
    klee_warning_once(
        nullptr,
        "Symbolic memory read will send the following array of %zu bytes to "
        "the constraint solver -- large symbolic arrays may cause significant "
        "performance issues: %s",
        sizeBound, allocInfo.c_str());
  }

  return ReadExpr::create(getUpdates(), ZExtExpr::create(offset, Expr::Int32));
}

void ObjectStatePlane::write8(size_t offset, uint8_t value) {
  //assert(read_only == false && "writing to read-only object!");
  if (offset >= sizeBound)
    sizeBound = offset + 1;
  if (concreteStore.size() <= offset)
    concreteStore.resize(sizeBound, initialValue);
  concreteStore[offset] = value;
  setKnownSymbolic(offset, 0);

  markByteConcrete(offset);
  markByteUnflushed(offset);
}

void ObjectStatePlane::write8(size_t offset, ref<Expr> value) {
  // can happen when ExtractExpr special cases
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    write8(offset, (uint8_t) CE->getZExtValue(8));
  } else {
    if (offset >= sizeBound)
      sizeBound = offset + 1;
    setKnownSymbolic(offset, value.get());
      
    markByteSymbolic(offset);
    markByteUnflushed(offset);
  }
}

void ObjectStatePlane::write8(Executor &executor, ExecutionState &state, ref<Expr> offset, ref<Expr> value) {
  assert(!isa<ConstantExpr>(offset) &&
         "constant offset passed to symbolic write8");
  flushForWrite();

  if (sizeBound > 4096) {
    std::string allocInfo;
    parent->getObject()->getAllocInfo(allocInfo);
    klee_warning_once(
        nullptr,
        "Symbolic memory write will send the following array of %zu bytes to "
        "the constraint solver -- large symbolic arrays may cause significant "
        "performance issues: %s",
        sizeBound, allocInfo.c_str());
  }

  updates.extend(ZExtExpr::create(offset, Expr::Int32), value);
}

/***/

ref<Expr> ObjectStatePlane::read(Executor &executor, ExecutionState &state,
                            ref<Expr> offset, Expr::Width width) const {
  // Truncate offset to 32-bits.
  offset = ZExtExpr::create(offset, Expr::Int32);

  // Check for reads at constant offsets.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(offset))
    return read(CE->getZExtValue(32), width);

  // Treat bool specially, it is the only non-byte sized write we allow.
  if (width == Expr::Bool)
    return ExtractExpr::create(read8(executor, state, offset), 0, Expr::Bool);

  // Otherwise, follow the slow general case.
  size_t NumBytes = width / 8;
  assert(width == NumBytes * 8 && "Invalid read size!");
  ref<Expr> Res(0);
  for (size_t i = 0; i != NumBytes; ++i) {
    size_t idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    ref<Expr> Byte =
        read8(executor, state,
              AddExpr::create(offset, ConstantExpr::create(idx, Expr::Int32)));
    Res = i ? ConcatExpr::create(Byte, Res) : Byte;
  }

  return Res;
}

ref<Expr> ObjectStatePlane::read(size_t offset, Expr::Width width) const {
  // Treat bool specially, it is the only non-byte sized write we allow.
  if (width == Expr::Bool)
    return ExtractExpr::create(read8(offset), 0, Expr::Bool);

  // Otherwise, follow the slow general case.
  size_t NumBytes = width / 8;
  assert(width == NumBytes * 8 && "Invalid width for read size!");
  ref<Expr> Res(0);
  for (size_t i = 0; i != NumBytes; ++i) {
    size_t idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    ref<Expr> Byte = read8(offset + idx);
    Res = i ? ConcatExpr::create(Byte, Res) : Byte;
  }

  return Res;
}

void ObjectStatePlane::write(Executor &executor, ExecutionState &state,
                        ref<Expr> offset, ref<Expr> value) {
  // Truncate offset to 32-bits.
  offset = ZExtExpr::create(offset, Expr::Int32);

  // Check for writes at constant offsets.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(offset)) {
    write(CE->getZExtValue(32), value);
    return;
  }

  // Treat bool specially, it is the only non-byte sized write we allow.
  Expr::Width w = value->getWidth();
  if (w == Expr::Bool) {
    write8(executor, state, offset, ZExtExpr::create(value, Expr::Int8));
    return;
  }

  // Otherwise, follow the slow general case.
  size_t NumBytes = w / 8;
  assert(w == NumBytes * 8 && "Invalid write size!");
  for (size_t i = 0; i != NumBytes; ++i) {
    size_t idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(executor, state, AddExpr::create(offset, ConstantExpr::create(idx, Expr::Int32)),
           ExtractExpr::create(value, 8 * i, Expr::Int8));
  }
}

void ObjectStatePlane::write(size_t offset, ref<Expr> value) {
  // Check for writes of constant values.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    Expr::Width w = CE->getWidth();
    if (w <= 64 && klee::bits64::isPowerOfTwo(w)) {
      uint64_t val = CE->getZExtValue();
      switch (w) {
      default: assert(0 && "Invalid write size!");
      case  Expr::Bool:
      case  Expr::Int8:  write8(offset, val); return;
      case Expr::Int16: write16(offset, val); return;
      case Expr::Int32: write32(offset, val); return;
      case Expr::Int64: write64(offset, val); return;
      }
    }
  }

  // Treat bool specially, it is the only non-byte sized write we allow.
  Expr::Width w = value->getWidth();
  if (w == Expr::Bool) {
    write8(offset, ZExtExpr::create(value, Expr::Int8));
    return;
  }

  // Otherwise, follow the slow general case.
  size_t NumBytes = w / 8;
  assert(w == NumBytes * 8 && "Invalid write size!");
  for (size_t i = 0; i != NumBytes; ++i) {
    size_t idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, ExtractExpr::create(value, 8 * i, Expr::Int8));
  }
} 

void ObjectStatePlane::write16(size_t offset, uint16_t value) {
  size_t NumBytes = 2;
  for (size_t i = 0; i != NumBytes; ++i) {
    size_t idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, (uint8_t) (value >> (8 * i)));
  }
}

void ObjectStatePlane::write32(size_t offset, uint32_t value) {
  size_t NumBytes = 4;
  for (size_t i = 0; i != NumBytes; ++i) {
    size_t idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, (uint8_t) (value >> (8 * i)));
  }
}

void ObjectStatePlane::write64(size_t offset, uint64_t value) {
  size_t NumBytes = 8;
  for (size_t i = 0; i != NumBytes; ++i) {
    size_t idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, (uint8_t) (value >> (8 * i)));
  }
}

void ObjectStatePlane::print() const {
  llvm::errs() << "-- ObjectState --\n";
  llvm::errs() << "\tMemoryObject ID: " << parent->getObject()->id << "\n";
  llvm::errs() << "\tRoot Object: " << updates.root << "\n";
  llvm::errs() << "\tSize: " << sizeBound << "\n";

  llvm::errs() << "\tBytes:\n";
  for (unsigned i=0; i<sizeBound; i++) {
    llvm::errs() << "\t\t["<<i<<"]"
               << " concrete? " << isByteConcrete(i)
               << " known-sym? " << isByteKnownSymbolic(i)
               << " unflushed? " << isByteUnflushed(i) << " = ";
    ref<Expr> e = read8(i);
    llvm::errs() << e << "\n";
  }

  llvm::errs() << "\tUpdates:\n";
  for (const auto *un = updates.head.get(); un; un = un->next.get()) {
    llvm::errs() << "\t\t[" << un->index << "] = " << un->value << "\n";
  }
}

/****/

ObjectState::ObjectState(const MemoryObject *mo)
  : copyOnWriteOwner(0),
    object(mo),
    readOnly(false),
    segmentPlane(nullptr),
    offsetPlane(new ObjectStatePlane(this)) {
}


ObjectState::ObjectState(const MemoryObject *mo, const Array *array)
  : copyOnWriteOwner(0),
    object(mo),
    readOnly(false),
    segmentPlane(nullptr),
    offsetPlane(new ObjectStatePlane(this, array)) {
}

ObjectState::ObjectState(const ObjectState &os)
  : copyOnWriteOwner(0),
    object(os.object),
    readOnly(false),
    segmentPlane(nullptr),
    offsetPlane(new ObjectStatePlane(this, *os.offsetPlane)) {
  if (os.segmentPlane)
    segmentPlane = new ObjectStatePlane(this, *os.segmentPlane);
}

ObjectState::ObjectState(const ObjectState &os, const MemoryObject *mo)
  : ObjectState(os) {
    object = mo;
}

ObjectState::~ObjectState() {
  if (segmentPlane)
    delete segmentPlane;
  delete offsetPlane;
}

KValue ObjectState::read8(unsigned offset) const {
  ref<Expr> segment;
  if (segmentPlane) {
    segment = segmentPlane->read8(offset);
  } else {
    segment = ConstantExpr::alloc(0, Expr::Int8);
  }
  ref<Expr> value = offsetPlane->read8(offset);
  return KValue(segment, value);
}

KValue ObjectState::read(unsigned offset, Expr::Width width) const {
  ref<Expr> segment;
  if (segmentPlane) {
    segment = segmentPlane->read(offset, width);
  } else {
    segment = ConstantExpr::alloc(0, width);
  }
  ref<Expr> value = offsetPlane->read(offset, width);
  return KValue(segment, value);
}

KValue ObjectState::read(ref<Expr> offset, Expr::Width width) const {
  ref<Expr> segment;
  if (segmentPlane) {
    segment = segmentPlane->read(offset, width);
  } else {
    segment = ConstantExpr::alloc(0, width);
  }
  ref<Expr> value = offsetPlane->read(offset, width);
  return KValue(segment, value);
}

bool ObjectState::prepareSegmentPlane(bool nonzero) {
  if (!segmentPlane) {
    if (nonzero) {
      segmentPlane = new ObjectStatePlane(this);
      return true;
    }
    return false;
  }
  return true;
}

bool ObjectState::prepareSegmentPlane(ref<Expr> segment) {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(segment))
    return prepareSegmentPlane(!CE->isZero());
  return prepareSegmentPlane(true);
}

void ObjectState::write8(unsigned offset, uint8_t segment, uint8_t value) {
  if (prepareSegmentPlane(segment))
    segmentPlane->write8(offset, segment);
  offsetPlane->write8(offset, value);
}

void ObjectState::write16(unsigned offset, uint16_t segment, uint16_t value) {
  if (prepareSegmentPlane(segment))
    segmentPlane->write16(offset, segment);
  offsetPlane->write16(offset, value);
}

void ObjectState::write32(unsigned offset, uint32_t segment, uint32_t value) {
  if (prepareSegmentPlane(segment))
    segmentPlane->write32(offset, segment);
  offsetPlane->write32(offset, value);
}

void ObjectState::write64(unsigned offset, uint64_t segment, uint64_t value) {
  if (prepareSegmentPlane(segment))
    segmentPlane->write64(offset, segment);
  offsetPlane->write64(offset, value);
}

void ObjectState::write(unsigned offset, const KValue& value) {
  if (prepareSegmentPlane(value.getSegment()))
    segmentPlane->write(offset, value.getSegment());
  offsetPlane->write(offset, value.getOffset());
}

void ObjectState::write(ref<Expr> offset, const KValue& value) {
  if (prepareSegmentPlane(value.getSegment()))
    segmentPlane->write(offset, value.getSegment());
  offsetPlane->write(offset, value.getOffset());
}

void ObjectState::initializeToZero() {
  offsetPlane->initializeToZero();
}

void ObjectState::initializeToRandom() {
  offsetPlane->initializeToRandom();
}

ArrayCache* ObjectState::getArrayCache() const {
  assert(object && "object was NULL");
  return object->parent->getArrayCache();
}
