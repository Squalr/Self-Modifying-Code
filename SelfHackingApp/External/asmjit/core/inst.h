// [AsmJit]
// Machine Code Generation for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

#ifndef _ASMJIT_CORE_INST_H
#define _ASMJIT_CORE_INST_H

#include "../core/cpuinfo.h"
#include "../core/operand.h"
#include "../core/string.h"
#include "../core/support.h"

ASMJIT_BEGIN_NAMESPACE

//! \addtogroup asmjit_core
//! \{

// ============================================================================
// [asmjit::InstInfo]
// ============================================================================

// TODO: Finalize instruction info and make more x86::InstDB methods/structs private.

/*

struct InstInfo {
  //! Architecture agnostic attributes.
  enum Attributes : uint32_t {


  };

  //! Instruction attributes.
  uint32_t _attributes;

  inline void reset() noexcept { memset(this, 0, sizeof(*this)); }

  inline uint32_t attributes() const noexcept { return _attributes; }
  inline bool hasAttribute(uint32_t attr) const noexcept { return (_attributes & attr) != 0; }
};

//! Gets attributes of the given instruction.
ASMJIT_API Error queryCommonInfo(uint32_t archId, uint32_t instId, InstInfo& out) noexcept;

*/

// ============================================================================
// [asmjit::InstRWInfo / OpRWInfo]
// ============================================================================

//! Read/Write information related to a single operand, used by `InstRWInfo`.
struct OpRWInfo {
  //! Read/Write flags, see `OpRWInfo::Flags`.
  uint32_t _opFlags;
  //! Physical register index, if required.
  uint8_t _physId;
  //! Size of a possible memory operand that can replace a register operand.
  uint8_t _rmSize;
  //! Reserved for future use.
  uint8_t _reserved[2];
  //! Read bit-mask where each bit represents one byte read from Reg/Mem.
  uint64_t _readByteMask;
  //! Write bit-mask where each bit represents one byte written to Reg/Mem.
  uint64_t _writeByteMask;
  //! Zero/Sign extend bit-mask where each bit represents one byte written to Reg/Mem.
  uint64_t _extendByteMask;

  //! Flags describe how the operand is accessed and some additional information.
  enum Flags : uint32_t {
    //! Operand is read.
    //!
    //! \note This flag must be `0x00000001`.
    kRead = 0x00000001u,

    //! Operand is written.
    //!
    //! \note This flag must be `0x00000002`.
    kWrite = 0x00000002u,

    //! Operand is both read and written.
    //!
    //! \note This combination of flags must be `0x00000003`.
    kRW = 0x00000003u,

    //! Register operand can be replaced by a memory operand.
    kRegMem = 0x00000004u,

    //! The `extendByteMask()` represents a zero extension.
    kZExt = 0x00000010u,

    //! Register operand must use `physId()`.
    kRegPhysId = 0x00000100u,
    //! Base register of a memory operand must use `physId()`.
    kMemPhysId = 0x00000200u,

    //! This memory operand is only used to encode registers and doesn't access memory.
    //!
    //! X86 Specific
    //! ------------
    //!
    //! Instructions that use such feature include BNDLDX, BNDSTX, and LEA.
    kMemFake = 0x000000400u,

    //! Base register of the memory operand will be read.
    kMemBaseRead = 0x00001000u,
    //! Base register of the memory operand will be written.
    kMemBaseWrite = 0x00002000u,
    //! Base register of the memory operand will be read & written.
    kMemBaseRW = 0x00003000u,

    //! Index register of the memory operand will be read.
    kMemIndexRead = 0x00004000u,
    //! Index register of the memory operand will be written.
    kMemIndexWrite = 0x00008000u,
    //! Index register of the memory operand will be read & written.
    kMemIndexRW = 0x0000C000u,

    //! Base register of the memory operand will be modified before the operation.
    kMemBasePreModify = 0x00010000u,
    //! Base register of the memory operand will be modified after the operation.
    kMemBasePostModify = 0x00020000u
  };

  static_assert(kRead  == 0x1, "OpRWInfo::kRead flag must be 0x1");
  static_assert(kWrite == 0x2, "OpRWInfo::kWrite flag must be 0x2");
  static_assert(kRegMem == 0x4, "OpRWInfo::kRegMem flag must be 0x4");

  //! \name Reset
  //! \{

  inline void reset() noexcept { memset(this, 0, sizeof(*this)); }
  inline void reset(uint32_t opFlags, uint32_t regSize, uint32_t physId = BaseReg::kIdBad) noexcept {
    _opFlags = opFlags;
    _physId = uint8_t(physId);
    _rmSize = uint8_t((opFlags & kRegMem) ? regSize : uint32_t(0));
    _resetReserved();

    uint64_t mask = Support::lsbMask<uint64_t>(regSize);
    _readByteMask = opFlags & kRead ? mask : uint64_t(0);
    _writeByteMask = opFlags & kWrite ? mask : uint64_t(0);
    _extendByteMask = 0;
  }

  inline void _resetReserved() noexcept {
    memset(_reserved, 0, sizeof(_reserved));
  }

  //! \}

  //! \name Operand Flags
  //! \{

  inline uint32_t opFlags() const noexcept { return _opFlags; }
  inline bool hasOpFlag(uint32_t flag) const noexcept { return (_opFlags & flag) != 0; }

  inline void addOpFlags(uint32_t flags) noexcept { _opFlags |= flags; }
  inline void clearOpFlags(uint32_t flags) noexcept { _opFlags &= ~flags; }

  inline bool isRead() const noexcept { return hasOpFlag(kRead); }
  inline bool isWrite() const noexcept { return hasOpFlag(kWrite); }
  inline bool isReadWrite() const noexcept { return (_opFlags & kRW) == kRW; }
  inline bool isReadOnly() const noexcept { return (_opFlags & kRW) == kRead; }
  inline bool isWriteOnly() const noexcept { return (_opFlags & kRW) == kWrite; }
  inline bool isRm() const noexcept { return hasOpFlag(kRegMem); }
  inline bool isZExt() const noexcept { return hasOpFlag(kZExt); }

  //! \}

  //! \name Physical Register ID
  //! \{

  inline uint32_t physId() const noexcept { return _physId; }
  inline bool hasPhysId() const noexcept { return _physId != BaseReg::kIdBad; }
  inline void setPhysId(uint32_t physId) noexcept { _physId = uint8_t(physId); }

  //! \}

  //! \name Reg/Mem
  //! \{

  inline uint32_t rmSize() const noexcept { return _rmSize; }
  inline void setRmSize(uint32_t rmSize) noexcept { _rmSize = uint8_t(rmSize); }

  //! \}

  //! \name Read & Write Masks
  //! \{

  inline uint64_t readByteMask() const noexcept { return _readByteMask; }
  inline uint64_t writeByteMask() const noexcept { return _writeByteMask; }
  inline uint64_t extendByteMask() const noexcept { return _extendByteMask; }

  inline void setReadByteMask(uint64_t mask) noexcept { _readByteMask = mask; }
  inline void setWriteByteMask(uint64_t mask) noexcept { _writeByteMask = mask; }
  inline void setExtendByteMask(uint64_t mask) noexcept { _extendByteMask = mask; }

  //! \}
};

//! Read/Write information of an instruction.
struct InstRWInfo {
  //! Instruction flags.
  uint32_t _instFlags;
  //! Mask of flags read.
  uint32_t _readFlags;
  //! Mask of flags written.
  uint32_t _writeFlags;
  //! Count of operands.
  uint8_t _opCount;
  //! CPU feature required for replacing register operand with memory operand.
  uint8_t _rmFeature;
  //! Reserved for future use.
  uint8_t _reserved[19];
  //! Read/Write onfo of extra register (rep{} or kz{}).
  OpRWInfo _extraReg;
  //! Read/Write info of instruction operands.
  OpRWInfo _operands[Globals::kMaxOpCount];

  inline void reset() noexcept { memset(this, 0, sizeof(*this)); }

  inline uint32_t instFlags() const noexcept { return _instFlags; }
  inline bool hasInstFlag(uint32_t flag) const noexcept { return (_instFlags & flag) != 0; }

  inline uint32_t opCount() const noexcept { return _opCount; }

  inline uint32_t readFlags() const noexcept { return _readFlags; }
  inline uint32_t writeFlags() const noexcept { return _writeFlags; }

  //! Returns the CPU feature required to replace a register operand with memory
  //! operand. If the returned feature is zero (none) then this instruction
  //! either doesn't provide memory operand combination or there is no extra
  //! CPU feature required.
  //!
  //! X86 Specific
  //! ------------
  //!
  //! Some AVX+ instructions may require extra features for replacing registers
  //! with memory operands, for example VPSLLDQ instruction only supports
  //! 'reg/reg/imm' combination on AVX/AVX2 capable CPUs and requires AVX-512 for
  //! 'reg/mem/imm' combination.
  inline uint32_t rmFeature() const noexcept { return _rmFeature; }

  inline const OpRWInfo& extraReg() const noexcept { return _extraReg; }
  inline const OpRWInfo* operands() const noexcept { return _operands; }

  inline const OpRWInfo& operand(size_t index) const noexcept {
    ASMJIT_ASSERT(index < Globals::kMaxOpCount);
    return _operands[index];
  }
};

// ============================================================================
// [asmjit::BaseInst]
// ============================================================================

//! Instruction id, options, and extraReg in a single structure. This structure
//! exists mainly to simplify analysis and validation API that requires `BaseInst`
//! and `Operand[]` array.
class BaseInst {
public:
  //! Instruction id.
  uint32_t _id;
  //! Instruction options.
  uint32_t _options;
  //! Extra register used by instruction (either REP register or AVX-512 selector).
  RegOnly _extraReg;

  enum Id : uint32_t {
    //! Invalid or uninitialized instruction id.
    kIdNone               = 0x00000000u,
    //! Abstract instruction (BaseBuilder and BaseCompiler).
    kIdAbstract           = 0x80000000u
  };

  enum Options : uint32_t {
    //! Used internally by emitters for handling errors and rare cases.
    kOptionReserved       = 0x00000001u,

    //! Used only by Assembler to mark that `_op4` and `_op5` are used (internal).
    kOptionOp4Op5Used     = 0x00000002u,

    //! Prevents following a jump during compilation (BaseCompiler).
    kOptionUnfollow       = 0x00000010u,

    //! Overwrite the destination operand(s) (BaseCompiler).
    //!
    //! Hint that is important for register liveness analysis. It tells the
    //! compiler that the destination operand will be overwritten now or by
    //! adjacent instructions. BaseCompiler knows when a register is completely
    //! overwritten by a single instruction, for example you don't have to
    //! mark "movaps" or "pxor x, x", however, if a pair of instructions is
    //! used and the first of them doesn't completely overwrite the content
    //! of the destination, BaseCompiler fails to mark that register as dead.
    //!
    //! X86 Specific
    //! ------------
    //!
    //!   - All instructions that always overwrite at least the size of the
    //!     register the virtual-register uses , for example "mov", "movq",
    //!     "movaps" don't need the overwrite option to be used - conversion,
    //!     shuffle, and other miscellaneous instructions included.
    //!
    //!   - All instructions that clear the destination register if all operands
    //!     are the same, for example "xor x, x", "pcmpeqb x x", etc...
    //!
    //!   - Consecutive instructions that partially overwrite the variable until
    //!     there is no old content require `BaseCompiler::overwrite()` to be used.
    //!     Some examples (not always the best use cases thought):
    //!
    //!     - `movlps xmm0, ?` followed by `movhps xmm0, ?` and vice versa
    //!     - `movlpd xmm0, ?` followed by `movhpd xmm0, ?` and vice versa
    //!     - `mov al, ?` followed by `and ax, 0xFF`
    //!     - `mov al, ?` followed by `mov ah, al`
    //!     - `pinsrq xmm0, ?, 0` followed by `pinsrq xmm0, ?, 1`
    //!
    //!   - If allocated variable is used temporarily for scalar operations. For
    //!     example if you allocate a full vector like `x86::Compiler::newXmm()`
    //!     and then use that vector for scalar operations you should use
    //!     `overwrite()` directive:
    //!
    //!     - `sqrtss x, y` - only LO element of `x` is changed, if you don't
    //!       use HI elements, use `compiler.overwrite().sqrtss(x, y)`.
    kOptionOverwrite      = 0x00000020u,

    //! Emit short-form of the instruction.
    kOptionShortForm      = 0x00000040u,
    //! Emit long-form of the instruction.
    kOptionLongForm       = 0x00000080u,

    //! Conditional jump is likely to be taken.
    kOptionTaken          = 0x00000100u,
    //! Conditional jump is unlikely to be taken.
    kOptionNotTaken       = 0x00000200u
  };

  //! Control type.
  enum ControlType : uint32_t {
    //! No control type (doesn't jump).
    kControlNone = 0u,
    //! Unconditional jump.
    kControlJump = 1u,
    //! Conditional jump (branch).
    kControlBranch = 2u,
    //! Function call.
    kControlCall = 3u,
    //! Function return.
    kControlReturn = 4u
  };

  //! \name Construction & Destruction
  //! \{

  inline explicit BaseInst(uint32_t id = 0, uint32_t options = 0) noexcept
    : _id(id),
      _options(options),
      _extraReg() {}

  inline BaseInst(uint32_t id, uint32_t options, const RegOnly& extraReg) noexcept
    : _id(id),
      _options(options),
      _extraReg(extraReg) {}

  inline BaseInst(uint32_t id, uint32_t options, const BaseReg& extraReg) noexcept
    : _id(id),
      _options(options),
      _extraReg { extraReg.signature(), extraReg.id() } {}

  //! \}

  //! \name Instruction ID
  //! \{

  inline uint32_t id() const noexcept { return _id; }
  inline void setId(uint32_t id) noexcept { _id = id; }
  inline void resetId() noexcept { _id = 0; }

  //! \}

  //! \name Instruction Options
  //! \{

  inline uint32_t options() const noexcept { return _options; }
  inline void setOptions(uint32_t options) noexcept { _options = options; }
  inline void addOptions(uint32_t options) noexcept { _options |= options; }
  inline void clearOptions(uint32_t options) noexcept { _options &= ~options; }
  inline void resetOptions() noexcept { _options = 0; }

  //! \}

  //! \name Extra Register
  //! \{

  inline bool hasExtraReg() const noexcept { return _extraReg.isReg(); }
  inline RegOnly& extraReg() noexcept { return _extraReg; }
  inline const RegOnly& extraReg() const noexcept { return _extraReg; }
  inline void setExtraReg(const BaseReg& reg) noexcept { _extraReg.init(reg); }
  inline void setExtraReg(const RegOnly& reg) noexcept { _extraReg.init(reg); }
  inline void resetExtraReg() noexcept { _extraReg.reset(); }

  //! \}
};

// ============================================================================
// [asmjit::InstAPI]
// ============================================================================

//! Instruction API.
namespace InstAPI {

#ifndef ASMJIT_NO_TEXT
//! Appends the name of the instruction specified by `instId` and `instOptions`
//! into the `output` string.
//!
//! \note Instruction options would only affect instruction prefix & suffix,
//! other options would be ignored. If `instOptions` is zero then only raw
//! instruction name (without any additional text) will be appended.
ASMJIT_API Error instIdToString(uint32_t archId, uint32_t instId, String& output) noexcept;

//! Parses an instruction name in the given string `s`. Length is specified
//! by `len` argument, which can be `SIZE_MAX` if `s` is known to be null
//! terminated.
//!
//! The output is stored in `instId`.
ASMJIT_API uint32_t stringToInstId(uint32_t archId, const char* s, size_t len) noexcept;
#endif // !ASMJIT_NO_TEXT

#ifndef ASMJIT_NO_VALIDATION
//! Validates the given instruction.
ASMJIT_API Error validate(uint32_t archId, const BaseInst& inst, const Operand_* operands, uint32_t opCount) noexcept;
#endif // !ASMJIT_NO_VALIDATION

#ifndef ASMJIT_NO_INTROSPECTION
//! Gets Read/Write information of the given instruction.
ASMJIT_API Error queryRWInfo(uint32_t archId, const BaseInst& inst, const Operand_* operands, uint32_t opCount, InstRWInfo& out) noexcept;

//! Gets CPU features required by the given instruction.
ASMJIT_API Error queryFeatures(uint32_t archId, const BaseInst& inst, const Operand_* operands, uint32_t opCount, BaseFeatures& out) noexcept;
#endif // !ASMJIT_NO_INTROSPECTION

} // {InstAPI}

//! \}

ASMJIT_END_NAMESPACE

#endif // _ASMJIT_CORE_INST_H
