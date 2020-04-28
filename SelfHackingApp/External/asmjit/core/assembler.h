// [AsmJit]
// Machine Code Generation for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

#ifndef _ASMJIT_CORE_ASSEMBLER_H
#define _ASMJIT_CORE_ASSEMBLER_H

#include "../core/codeholder.h"
#include "../core/datatypes.h"
#include "../core/emitter.h"
#include "../core/operand.h"

ASMJIT_BEGIN_NAMESPACE

//! \addtogroup asmjit_core
//! \{

// ============================================================================
// [asmjit::BaseAssembler]
// ============================================================================

//! Base encoder (assembler).
class ASMJIT_VIRTAPI BaseAssembler : public BaseEmitter {
public:
  ASMJIT_NONCOPYABLE(BaseAssembler)
  typedef BaseEmitter Base;

  //! Current section where the assembling happens.
  Section* _section;
  //! Start of the CodeBuffer of the current section.
  uint8_t* _bufferData;
  //! End (first invalid byte) of the current section.
  uint8_t* _bufferEnd;
  //! Pointer in the CodeBuffer of the current section.
  uint8_t* _bufferPtr;
  //! 5th operand data, used only temporarily.
  Operand_ _op4;
  //! 6th operand data, used only temporarily.
  Operand_ _op5;

  //! \name Construction & Destruction
  //! \{

  //! Creates a new `BaseAssembler` instance.
  ASMJIT_API BaseAssembler() noexcept;
  //! Destroys the `BaseAssembler` instance.
  ASMJIT_API virtual ~BaseAssembler() noexcept;

  //! \}

  //! \name Code-Buffer Management
  //! \{

  //! Returns the capacity of the current CodeBuffer.
  inline size_t bufferCapacity() const noexcept { return (size_t)(_bufferEnd - _bufferData); }
  //! Returns the number of remaining bytes in the current CodeBuffer.
  inline size_t remainingSpace() const noexcept { return (size_t)(_bufferEnd - _bufferPtr); }

  //! Returns the current position in the CodeBuffer.
  inline size_t offset() const noexcept { return (size_t)(_bufferPtr - _bufferData); }
  //! Sets the current position in the CodeBuffer to `offset`.
  //!
  //! \note The `offset` cannot be outside of the buffer size (even if it's
  //! within buffer's capacity).
  ASMJIT_API Error setOffset(size_t offset);

  //! Returns the start of the CodeBuffer in the current section.
  inline uint8_t* bufferData() const noexcept { return _bufferData; }
  //! Returns the end (first invalid byte) in the current section.
  inline uint8_t* bufferEnd() const noexcept { return _bufferEnd; }
  //! Returns the current pointer in the CodeBuffer in the current section.
  inline uint8_t* bufferPtr() const noexcept { return _bufferPtr; }

  //! \}

  //! \name Section Management
  //! \{

  inline Section* currentSection() const noexcept { return _section; }

  ASMJIT_API Error section(Section* section) override;

  //! \}

  //! \name Label Management
  //! \{

  ASMJIT_API Label newLabel() override;
  ASMJIT_API Label newNamedLabel(const char* name, size_t nameSize = SIZE_MAX, uint32_t type = Label::kTypeGlobal, uint32_t parentId = Globals::kInvalidId) override;
  ASMJIT_API Error bind(const Label& label) override;

  //! \}

  //! \cond INTERNAL
  //! \name Emit
  //! \{

  using BaseEmitter::_emit;

  ASMJIT_API Error _emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3, const Operand_& o4, const Operand_& o5) override;
  ASMJIT_API Error _emitOpArray(uint32_t instId, const Operand_* operands, size_t count) override;

protected:
  #ifndef ASMJIT_NO_LOGGING
  void _emitLog(
    uint32_t instId, uint32_t options, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3,
    uint32_t relSize, uint32_t immSize, uint8_t* afterCursor);

  Error _emitFailed(
    Error err,
    uint32_t instId, uint32_t options, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3);
  #else
  inline Error _emitFailed(
    uint32_t err,
    uint32_t instId, uint32_t options, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) {

    ASMJIT_UNUSED(instId);
    ASMJIT_UNUSED(options);
    ASMJIT_UNUSED(o0);
    ASMJIT_UNUSED(o1);
    ASMJIT_UNUSED(o2);
    ASMJIT_UNUSED(o3);

    resetInstOptions();
    resetInlineComment();
    return reportError(err);
  }
  #endif
public:
  //! \}
  //! \endcond

  //! \name Embed
  //! \{

  ASMJIT_API Error embed(const void* data, uint32_t dataSize) override;
  ASMJIT_API Error embedLabel(const Label& label) override;
  ASMJIT_API Error embedLabelDelta(const Label& label, const Label& base, uint32_t dataSize) override;
  ASMJIT_API Error embedConstPool(const Label& label, const ConstPool& pool) override;

  //! \}

  //! \name Comment
  //! \{

  ASMJIT_API Error comment(const char* data, size_t size = SIZE_MAX) override;

  //! \}

  //! \name Events
  //! \{

  ASMJIT_API Error onAttach(CodeHolder* code) noexcept override;
  ASMJIT_API Error onDetach(CodeHolder* code) noexcept override;

  //! \}
};

//! \}

ASMJIT_END_NAMESPACE

#endif // _ASMJIT_CORE_ASSEMBLER_H
