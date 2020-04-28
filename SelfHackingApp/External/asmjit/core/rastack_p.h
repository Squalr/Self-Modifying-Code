// [AsmJit]
// Machine Code Generation for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

#ifndef _ASMJIT_CORE_RASTACK_P_H
#define _ASMJIT_CORE_RASTACK_P_H

#include "../core/build.h"
#ifndef ASMJIT_NO_COMPILER

#include "../core/radefs_p.h"

ASMJIT_BEGIN_NAMESPACE

//! \cond INTERNAL
//! \addtogroup asmjit_ra
//! \{

// ============================================================================
// [asmjit::RAStackSlot]
// ============================================================================

//! Stack slot.
struct RAStackSlot {
  enum Flags : uint32_t {
    // TODO: kFlagRegHome is apparently not used, but isRegHome() is.
    kFlagRegHome          = 0x00000001u, //!< Stack slot is register home slot.
    kFlagStackArg         = 0x00000002u  //!< Stack slot position matches argument passed via stack.
  };

  enum ArgIndex : uint32_t {
    kNoArgIndex = 0xFF
  };

  //! Base register used to address the stack.
  uint8_t _baseRegId;
  //! Minimum alignment required by the slot.
  uint8_t _alignment;
  //! Reserved for future use.
  uint8_t _reserved[2];
  //! Size of memory required by the slot.
  uint32_t _size;
  //! Slot flags.
  uint32_t _flags;

  //! Usage counter (one unit equals one memory access).
  uint32_t _useCount;
  //! Weight of the slot (calculated by `calculateStackFrame()`).
  uint32_t _weight;
  //! Stack offset (calculated by `calculateStackFrame()`).
  int32_t _offset;

  //! \name Accessors
  //! \{

  inline uint32_t baseRegId() const noexcept { return _baseRegId; }
  inline void setBaseRegId(uint32_t id) noexcept { _baseRegId = uint8_t(id); }

  inline uint32_t size() const noexcept { return _size; }
  inline uint32_t alignment() const noexcept { return _alignment; }

  inline uint32_t flags() const noexcept { return _flags; }
  inline void addFlags(uint32_t flags) noexcept { _flags |= flags; }
  inline bool isRegHome() const noexcept { return (_flags & kFlagRegHome) != 0; }
  inline bool isStackArg() const noexcept { return (_flags & kFlagStackArg) != 0; }

  inline uint32_t useCount() const noexcept { return _useCount; }
  inline void addUseCount(uint32_t n = 1) noexcept { _useCount += n; }

  inline uint32_t weight() const noexcept { return _weight; }
  inline void setWeight(uint32_t weight) noexcept { _weight = weight; }

  inline int32_t offset() const noexcept { return _offset; }
  inline void setOffset(int32_t offset) noexcept { _offset = offset; }

  //! \}
};

typedef ZoneVector<RAStackSlot*> RAStackSlots;

// ============================================================================
// [asmjit::RAStackAllocator]
// ============================================================================

//! Stack allocator.
class RAStackAllocator {
public:
  ASMJIT_NONCOPYABLE(RAStackAllocator)

  enum Size : uint32_t {
    kSize1     = 0,
    kSize2     = 1,
    kSize4     = 2,
    kSize8     = 3,
    kSize16    = 4,
    kSize32    = 5,
    kSize64    = 6,
    kSizeCount = 7
  };

  //! Allocator used to allocate internal data.
  ZoneAllocator* _allocator;
  //! Count of bytes used by all slots.
  uint32_t _bytesUsed;
  //! Calculated stack size (can be a bit greater than `_bytesUsed`).
  uint32_t _stackSize;
  //! Minimum stack alignment.
  uint32_t _alignment;
  //! Stack slots vector.
  RAStackSlots _slots;

  //! \name Construction / Destruction
  //! \{

  inline RAStackAllocator() noexcept
    : _allocator(nullptr),
      _bytesUsed(0),
      _stackSize(0),
      _alignment(1),
      _slots() {}

  inline void reset(ZoneAllocator* allocator) noexcept {
    _allocator = allocator;
    _bytesUsed = 0;
    _stackSize = 0;
    _alignment = 1;
    _slots.reset();
  }

  //! \}

  //! \name Accessors
  //! \{

  inline ZoneAllocator* allocator() const noexcept { return _allocator; }

  inline uint32_t bytesUsed() const noexcept { return _bytesUsed; }
  inline uint32_t stackSize() const noexcept { return _stackSize; }
  inline uint32_t alignment() const noexcept { return _alignment; }

  inline RAStackSlots& slots() noexcept { return _slots; }
  inline const RAStackSlots& slots() const noexcept { return _slots; }
  inline uint32_t slotCount() const noexcept { return _slots.size(); }

  //! \}

  //! \name Utilities
  //! \{

  RAStackSlot* newSlot(uint32_t baseRegId, uint32_t size, uint32_t alignment, uint32_t flags = 0) noexcept;

  Error calculateStackFrame() noexcept;
  Error adjustSlotOffsets(int32_t offset) noexcept;

  //! \}
};

//! \}
//! \endcond

ASMJIT_END_NAMESPACE

#endif // !ASMJIT_NO_COMPILER
#endif // _ASMJIT_CORE_RASTACK_P_H
