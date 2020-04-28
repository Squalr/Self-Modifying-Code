// [AsmJit]
// Machine Code Generation for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

#ifndef _ASMJIT_CORE_ZONESTACK_H
#define _ASMJIT_CORE_ZONESTACK_H

#include "../core/zone.h"

ASMJIT_BEGIN_NAMESPACE

//! \addtogroup asmjit_zone
//! \{

// ============================================================================
// [asmjit::ZoneStackBase]
// ============================================================================

//! Base class used by `ZoneStack<T>`.
class ZoneStackBase {
public:
  ASMJIT_NONCOPYABLE(ZoneStackBase)

  static constexpr uint32_t kBlockSize = ZoneAllocator::kHiMaxSize;

  struct Block {
    inline bool empty() const noexcept { return _start == _end; }
    inline Block* prev() const noexcept { return _link[Globals::kLinkLeft]; }
    inline Block* next() const noexcept { return _link[Globals::kLinkRight]; }

    inline void setPrev(Block* block) noexcept { _link[Globals::kLinkLeft] = block; }
    inline void setNext(Block* block) noexcept { _link[Globals::kLinkRight] = block; }

    template<typename T>
    inline T* start() const noexcept { return static_cast<T*>(_start); }
    template<typename T>
    inline void setStart(T* start) noexcept { _start = static_cast<void*>(start); }

    template<typename T>
    inline T* end() const noexcept { return (T*)_end; }
    template<typename T>
    inline void setEnd(T* end) noexcept { _end = (void*)end; }

    template<typename T>
    inline T* data() const noexcept { return (T*)((uint8_t*)(this) + sizeof(Block)); }

    template<typename T>
    inline bool canPrepend() const noexcept { return _start > data<void>(); }

    template<typename T>
    inline bool canAppend() const noexcept {
      size_t kNumBlockItems = (kBlockSize - sizeof(Block)) / sizeof(T);
      size_t kStartBlockIndex = sizeof(Block);
      size_t kEndBlockIndex = kStartBlockIndex + kNumBlockItems * sizeof(T);

      return (uintptr_t)_end <= ((uintptr_t)this + kEndBlockIndex - sizeof(T));
    }

    Block* _link[Globals::kLinkCount];   //!< Next and previous blocks.
    void* _start;                        //!< Pointer to the start of the array.
    void* _end;                          //!< Pointer to the end of the array.
  };

  //! Allocator used to allocate data.
  ZoneAllocator* _allocator;
  //! First and last blocks.
  Block* _block[Globals::kLinkCount];

  //! \name Construction / Destruction
  //! \{

  inline ZoneStackBase() noexcept {
    _allocator = nullptr;
    _block[0] = nullptr;
    _block[1] = nullptr;
  }
  inline ~ZoneStackBase() noexcept { reset(); }

  inline bool isInitialized() const noexcept { return _allocator != nullptr; }
  ASMJIT_API Error _init(ZoneAllocator* allocator, size_t middleIndex) noexcept;
  inline Error reset() noexcept { return _init(nullptr, 0); }

  //! \}

  //! \name Accessors
  //! \{

  //! Returns `ZoneAllocator` attached to this container.
  inline ZoneAllocator* allocator() const noexcept { return _allocator; }

  inline bool empty() const noexcept {
    ASMJIT_ASSERT(isInitialized());
    return _block[0]->start<void>() == _block[1]->end<void>();
  }

  //! \}

  //! \cond INTERNAL
  //! \name Internal
  //! \{

  ASMJIT_API Error _prepareBlock(uint32_t side, size_t initialIndex) noexcept;
  ASMJIT_API void _cleanupBlock(uint32_t side, size_t middleIndex) noexcept;

  //! \}
  //! \endcond
};

// ============================================================================
// [asmjit::ZoneStack<T>]
// ============================================================================

//! Zone allocated stack container.
template<typename T>
class ZoneStack : public ZoneStackBase {
public:
  ASMJIT_NONCOPYABLE(ZoneStack<T>)

  enum : uint32_t {
    kNumBlockItems   = uint32_t((kBlockSize - sizeof(Block)) / sizeof(T)),
    kStartBlockIndex = uint32_t(sizeof(Block)),
    kMidBlockIndex   = uint32_t(kStartBlockIndex + (kNumBlockItems / 2) * sizeof(T)),
    kEndBlockIndex   = uint32_t(kStartBlockIndex + (kNumBlockItems    ) * sizeof(T))
  };

  //! \name Construction / Destruction
  //! \{

  inline ZoneStack() noexcept {}
  inline ~ZoneStack() noexcept {}

  inline Error init(ZoneAllocator* allocator) noexcept { return _init(allocator, kMidBlockIndex); }

  //! \}

  //! \name Utilities
  //! \{

  ASMJIT_INLINE Error prepend(T item) noexcept {
    ASMJIT_ASSERT(isInitialized());
    Block* block = _block[Globals::kLinkFirst];

    if (!block->canPrepend<T>()) {
      ASMJIT_PROPAGATE(_prepareBlock(Globals::kLinkFirst, kEndBlockIndex));
      block = _block[Globals::kLinkFirst];
    }

    T* ptr = block->start<T>() - 1;
    ASMJIT_ASSERT(ptr >= block->data<T>() && ptr <= block->data<T>() + (kNumBlockItems - 1));
    *ptr = item;
    block->setStart<T>(ptr);
    return kErrorOk;
  }

  ASMJIT_INLINE Error append(T item) noexcept {
    ASMJIT_ASSERT(isInitialized());
    Block* block = _block[Globals::kLinkLast];

    if (!block->canAppend<T>()) {
      ASMJIT_PROPAGATE(_prepareBlock(Globals::kLinkLast, kStartBlockIndex));
      block = _block[Globals::kLinkLast];
    }

    T* ptr = block->end<T>();
    ASMJIT_ASSERT(ptr >= block->data<T>() && ptr <= block->data<T>() + (kNumBlockItems - 1));

    *ptr++ = item;
    block->setEnd(ptr);
    return kErrorOk;
  }

  ASMJIT_INLINE T popFirst() noexcept {
    ASMJIT_ASSERT(isInitialized());
    ASMJIT_ASSERT(!empty());

    Block* block = _block[Globals::kLinkFirst];
    ASMJIT_ASSERT(!block->empty());

    T* ptr = block->start<T>();
    T item = *ptr++;

    block->setStart(ptr);
    if (block->empty())
      _cleanupBlock(Globals::kLinkFirst, kMidBlockIndex);

    return item;
  }

  ASMJIT_INLINE T pop() noexcept {
    ASMJIT_ASSERT(isInitialized());
    ASMJIT_ASSERT(!empty());

    Block* block = _block[Globals::kLinkLast];
    ASMJIT_ASSERT(!block->empty());

    T* ptr = block->end<T>();
    T item = *--ptr;
    ASMJIT_ASSERT(ptr >= block->data<T>());
    ASMJIT_ASSERT(ptr >= block->start<T>());

    block->setEnd(ptr);
    if (block->empty())
      _cleanupBlock(Globals::kLinkLast, kMidBlockIndex);

    return item;
  }

  //! \}
};

//! \}

ASMJIT_END_NAMESPACE

#endif // _ASMJIT_CORE_ZONESTACK_H
