// [AsmJit]
// Machine Code Generation for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// ----------------------------------------------------------------------------
// IMPORTANT: AsmJit now uses an external instruction database to populate
// static tables within this file. Perform the following steps to regenerate
// all tables enclosed by ${...}:
//
//   1. Install node.js environment <https://nodejs.org>
//   2. Go to asmjit/tools directory
//   3. Get the latest asmdb from <https://github.com/asmjit/asmdb> and
//      copy/link the `asmdb` directory to `asmjit/tools/asmdb`.
//   4. Execute `node tablegen-x86.js`
//
// Instruction encoding and opcodes were added to the `x86inst.cpp` database
// manually in the past and they are not updated by the script as it became
// tricky. However, everything else is updated including instruction operands
// and tables required to validate them, instruction read/write information
// (including registers and flags), and all indexes to all tables.
// ----------------------------------------------------------------------------

#define ASMJIT_EXPORTS

#include "../core/build.h"
#ifdef ASMJIT_BUILD_X86

#include "../core/cpuinfo.h"
#include "../core/misc_p.h"
#include "../core/support.h"
#include "../x86/x86features.h"
#include "../x86/x86instapi_p.h"
#include "../x86/x86instdb_p.h"
#include "../x86/x86opcode_p.h"
#include "../x86/x86operand.h"

ASMJIT_BEGIN_SUB_NAMESPACE(x86)

// ============================================================================
// [asmjit::x86::InstInternal - Text]
// ============================================================================

#ifndef ASMJIT_NO_TEXT
Error InstInternal::instIdToString(uint32_t archId, uint32_t instId, String& output) noexcept {
  ASMJIT_UNUSED(archId);

  if (ASMJIT_UNLIKELY(!Inst::isDefinedId(instId)))
    return DebugUtils::errored(kErrorInvalidInstruction);

  const InstDB::InstInfo& info = InstDB::infoById(instId);
  return output.appendString(InstDB::_nameData + info._nameDataIndex);
}

uint32_t InstInternal::stringToInstId(uint32_t archId, const char* s, size_t len) noexcept {
  ASMJIT_UNUSED(archId);

  if (ASMJIT_UNLIKELY(!s))
    return Inst::kIdNone;

  if (len == SIZE_MAX)
    len = strlen(s);

  if (ASMJIT_UNLIKELY(len == 0 || len > InstDB::kMaxNameSize))
    return Inst::kIdNone;

  uint32_t prefix = uint32_t(s[0]) - 'a';
  if (ASMJIT_UNLIKELY(prefix > 'z' - 'a'))
    return Inst::kIdNone;

  uint32_t index = InstDB::instNameIndex[prefix].start;
  if (ASMJIT_UNLIKELY(!index))
    return Inst::kIdNone;

  const char* nameData = InstDB::_nameData;
  const InstDB::InstInfo* table = InstDB::_instInfoTable;

  const InstDB::InstInfo* base = table + index;
  const InstDB::InstInfo* end  = table + InstDB::instNameIndex[prefix].end;

  for (size_t lim = (size_t)(end - base); lim != 0; lim >>= 1) {
    const InstDB::InstInfo* cur = base + (lim >> 1);
    int result = Support::cmpInstName(nameData + cur[0]._nameDataIndex, s, len);

    if (result < 0) {
      base = cur + 1;
      lim--;
      continue;
    }

    if (result > 0)
      continue;

    return uint32_t((size_t)(cur - table));
  }

  return Inst::kIdNone;
}
#endif // !ASMJIT_NO_TEXT

// ============================================================================
// [asmjit::x86::InstInternal - Validate]
// ============================================================================

#ifndef ASMJIT_NO_VALIDATION
struct X86ValidationData {
  //! Allowed registers by reg-type (x86::Reg::kType...).
  uint32_t allowedRegMask[Reg::kTypeMax + 1];
  uint32_t allowedMemBaseRegs;
  uint32_t allowedMemIndexRegs;
};

#define VALUE(X) \
  (X == Reg::kTypeGpbLo) ? InstDB::kOpGpbLo : \
  (X == Reg::kTypeGpbHi) ? InstDB::kOpGpbHi : \
  (X == Reg::kTypeGpw  ) ? InstDB::kOpGpw   : \
  (X == Reg::kTypeGpd  ) ? InstDB::kOpGpd   : \
  (X == Reg::kTypeGpq  ) ? InstDB::kOpGpq   : \
  (X == Reg::kTypeXmm  ) ? InstDB::kOpXmm   : \
  (X == Reg::kTypeYmm  ) ? InstDB::kOpYmm   : \
  (X == Reg::kTypeZmm  ) ? InstDB::kOpZmm   : \
  (X == Reg::kTypeMm   ) ? InstDB::kOpMm    : \
  (X == Reg::kTypeKReg ) ? InstDB::kOpKReg  : \
  (X == Reg::kTypeSReg ) ? InstDB::kOpSReg  : \
  (X == Reg::kTypeCReg ) ? InstDB::kOpCReg  : \
  (X == Reg::kTypeDReg ) ? InstDB::kOpDReg  : \
  (X == Reg::kTypeSt   ) ? InstDB::kOpSt    : \
  (X == Reg::kTypeBnd  ) ? InstDB::kOpBnd   : \
  (X == Reg::kTypeRip  ) ? InstDB::kOpNone  : InstDB::kOpNone
static const uint32_t _x86OpFlagFromRegType[Reg::kTypeMax + 1] = { ASMJIT_LOOKUP_TABLE_32(VALUE, 0) };
#undef VALUE

#define REG_MASK_FROM_REG_TYPE_X86(X) \
  (X == Reg::kTypeGpbLo) ? 0x0000000Fu : \
  (X == Reg::kTypeGpbHi) ? 0x0000000Fu : \
  (X == Reg::kTypeGpw  ) ? 0x000000FFu : \
  (X == Reg::kTypeGpd  ) ? 0x000000FFu : \
  (X == Reg::kTypeGpq  ) ? 0x000000FFu : \
  (X == Reg::kTypeXmm  ) ? 0x000000FFu : \
  (X == Reg::kTypeYmm  ) ? 0x000000FFu : \
  (X == Reg::kTypeZmm  ) ? 0x000000FFu : \
  (X == Reg::kTypeMm   ) ? 0x000000FFu : \
  (X == Reg::kTypeKReg ) ? 0x000000FFu : \
  (X == Reg::kTypeSReg ) ? 0x0000007Eu : \
  (X == Reg::kTypeCReg ) ? 0x0000FFFFu : \
  (X == Reg::kTypeDReg ) ? 0x000000FFu : \
  (X == Reg::kTypeSt   ) ? 0x000000FFu : \
  (X == Reg::kTypeBnd  ) ? 0x0000000Fu : \
  (X == Reg::kTypeRip  ) ? 0x00000001u : 0u

#define REG_MASK_FROM_REG_TYPE_X64(X) \
  (X == Reg::kTypeGpbLo) ? 0x0000FFFFu : \
  (X == Reg::kTypeGpbHi) ? 0x0000000Fu : \
  (X == Reg::kTypeGpw  ) ? 0x0000FFFFu : \
  (X == Reg::kTypeGpd  ) ? 0x0000FFFFu : \
  (X == Reg::kTypeGpq  ) ? 0x0000FFFFu : \
  (X == Reg::kTypeXmm  ) ? 0xFFFFFFFFu : \
  (X == Reg::kTypeYmm  ) ? 0xFFFFFFFFu : \
  (X == Reg::kTypeZmm  ) ? 0xFFFFFFFFu : \
  (X == Reg::kTypeMm   ) ? 0x000000FFu : \
  (X == Reg::kTypeKReg ) ? 0x000000FFu : \
  (X == Reg::kTypeSReg ) ? 0x0000007Eu : \
  (X == Reg::kTypeCReg ) ? 0x0000FFFFu : \
  (X == Reg::kTypeDReg ) ? 0x0000FFFFu : \
  (X == Reg::kTypeSt   ) ? 0x000000FFu : \
  (X == Reg::kTypeBnd  ) ? 0x0000000Fu : \
  (X == Reg::kTypeRip  ) ? 0x00000001u : 0u

static const X86ValidationData _x86ValidationData = {
  { ASMJIT_LOOKUP_TABLE_32(REG_MASK_FROM_REG_TYPE_X86, 0) },
  (1u << Reg::kTypeGpw) | (1u << Reg::kTypeGpd) | (1u << Reg::kTypeRip) | (1u << Label::kLabelTag),
  (1u << Reg::kTypeGpw) | (1u << Reg::kTypeGpd) | (1u << Reg::kTypeXmm) | (1u << Reg::kTypeYmm) | (1u << Reg::kTypeZmm)
};

static const X86ValidationData _x64ValidationData = {
  { ASMJIT_LOOKUP_TABLE_32(REG_MASK_FROM_REG_TYPE_X64, 0) },
  (1u << Reg::kTypeGpd) | (1u << Reg::kTypeGpq) | (1u << Reg::kTypeRip) | (1u << Label::kLabelTag),
  (1u << Reg::kTypeGpd) | (1u << Reg::kTypeGpq) | (1u << Reg::kTypeXmm) | (1u << Reg::kTypeYmm) | (1u << Reg::kTypeZmm)
};

#undef REG_MASK_FROM_REG_TYPE_X64
#undef REG_MASK_FROM_REG_TYPE_X86

static ASMJIT_INLINE bool x86IsZmmOrM512(const Operand_& op) noexcept {
  return Reg::isZmm(op) || (op.isMem() && op.size() == 64);
}

static ASMJIT_INLINE bool x86CheckOSig(const InstDB::OpSignature& op, const InstDB::OpSignature& ref, bool& immOutOfRange) noexcept {
  // Fail if operand types are incompatible.
  uint32_t opFlags = op.opFlags;
  if ((opFlags & ref.opFlags) == 0) {
    // Mark temporarily `immOutOfRange` so we can return a more descriptive error later.
    if ((opFlags & InstDB::kOpAllImm) && (ref.opFlags & InstDB::kOpAllImm)) {
      immOutOfRange = true;
      return true;
    }

    return false;
  }

  // Fail if memory specific flags and sizes do not match the signature.
  uint32_t opMemFlags = op.memFlags;
  if (opMemFlags != 0) {
    uint32_t refMemFlags = ref.memFlags;
    if ((refMemFlags & opMemFlags) == 0)
      return false;

    if ((refMemFlags & InstDB::kMemOpBaseOnly) && !(opMemFlags & InstDB::kMemOpBaseOnly))
      return false;
  }

  // Specific register index.
  if (opFlags & InstDB::kOpAllRegs) {
    uint32_t refRegMask = ref.regMask;
    if (refRegMask && !(op.regMask & refRegMask))
      return false;
  }

  return true;
}

ASMJIT_FAVOR_SIZE Error InstInternal::validate(uint32_t archId, const BaseInst& inst, const Operand_* operands, uint32_t opCount) noexcept {
  // Only called when `archId` matches X86 family.
  ASMJIT_ASSERT(ArchInfo::isX86Family(archId));

  const X86ValidationData* vd;
  if (archId == ArchInfo::kIdX86)
    vd = &_x86ValidationData;
  else
    vd = &_x64ValidationData;

  uint32_t i;
  uint32_t mode = InstDB::modeFromArchId(archId);

  // Get the instruction data.
  uint32_t instId = inst.id();
  uint32_t options = inst.options();

  if (ASMJIT_UNLIKELY(!Inst::isDefinedId(instId)))
    return DebugUtils::errored(kErrorInvalidInstruction);

  const InstDB::InstInfo& instInfo = InstDB::infoById(instId);
  const InstDB::CommonInfo& commonInfo = instInfo.commonInfo();

  uint32_t iFlags = instInfo.flags();

  // --------------------------------------------------------------------------
  // [Validate LOCK|XACQUIRE|XRELEASE]
  // --------------------------------------------------------------------------

  const uint32_t kLockXAcqRel = Inst::kOptionXAcquire | Inst::kOptionXRelease;
  if (options & (Inst::kOptionLock | kLockXAcqRel)) {
    if (options & Inst::kOptionLock) {
      if (ASMJIT_UNLIKELY(!(iFlags & InstDB::kFlagLock) && !(options & kLockXAcqRel)))
        return DebugUtils::errored(kErrorInvalidLockPrefix);

      if (ASMJIT_UNLIKELY(opCount < 1 || !operands[0].isMem()))
        return DebugUtils::errored(kErrorInvalidLockPrefix);
    }

    if (options & kLockXAcqRel) {
      if (ASMJIT_UNLIKELY(!(options & Inst::kOptionLock) || (options & kLockXAcqRel) == kLockXAcqRel))
        return DebugUtils::errored(kErrorInvalidPrefixCombination);

      if (ASMJIT_UNLIKELY((options & Inst::kOptionXAcquire) && !(iFlags & InstDB::kFlagXAcquire)))
        return DebugUtils::errored(kErrorInvalidXAcquirePrefix);

      if (ASMJIT_UNLIKELY((options & Inst::kOptionXRelease) && !(iFlags & InstDB::kFlagXRelease)))
        return DebugUtils::errored(kErrorInvalidXReleasePrefix);
    }
  }

  // Validate REP and REPNE prefixes.
  const uint32_t kRepAny = Inst::kOptionRep | Inst::kOptionRepne;
  if (options & kRepAny) {
    if (ASMJIT_UNLIKELY((options & kRepAny) == kRepAny))
      return DebugUtils::errored(kErrorInvalidPrefixCombination);

    if (ASMJIT_UNLIKELY(!(iFlags & InstDB::kFlagRep)))
      return DebugUtils::errored(kErrorInvalidRepPrefix);
  }

  // --------------------------------------------------------------------------
  // [Translate Each Operand to the Corresponding OpSignature]
  // --------------------------------------------------------------------------

  InstDB::OpSignature oSigTranslated[Globals::kMaxOpCount];
  uint32_t combinedOpFlags = 0;
  uint32_t combinedRegMask = 0;
  const Mem* memOp = nullptr;

  for (i = 0; i < opCount; i++) {
    const Operand_& op = operands[i];
    if (op.opType() == Operand::kOpNone)
      break;

    uint32_t opFlags = 0;
    uint32_t memFlags = 0;
    uint32_t regMask = 0;

    switch (op.opType()) {
      case Operand::kOpReg: {
        uint32_t regType = op.as<BaseReg>().type();
        if (ASMJIT_UNLIKELY(regType >= Reg::kTypeCount))
          return DebugUtils::errored(kErrorInvalidRegType);

        opFlags = _x86OpFlagFromRegType[regType];
        if (ASMJIT_UNLIKELY(opFlags == 0))
          return DebugUtils::errored(kErrorInvalidRegType);

        // If `regId` is equal or greater than Operand::kVirtIdMin it means
        // that the register is virtual and its index will be assigned later
        // by the register allocator. We must pass unless asked to disallow
        // virtual registers.
        // TODO: We need an option to refuse virtual regs here.
        uint32_t regId = op.id();
        if (regId < Operand::kVirtIdMin) {
          if (ASMJIT_UNLIKELY(regId >= 32))
            return DebugUtils::errored(kErrorInvalidPhysId);

          if (ASMJIT_UNLIKELY(Support::bitTest(vd->allowedRegMask[regType], regId) == 0))
            return DebugUtils::errored(kErrorInvalidPhysId);

          regMask = Support::bitMask(regId);
          combinedRegMask |= regMask;
        }
        else {
          regMask = 0xFFFFFFFFu;
        }
        break;
      }

      // TODO: Validate base and index and combine these with `combinedRegMask`.
      case Operand::kOpMem: {
        const Mem& m = op.as<Mem>();
        memOp = &m;

        uint32_t memSize = m.size();
        uint32_t baseType = m.baseType();
        uint32_t indexType = m.indexType();

        if (m.segmentId() > 6)
          return DebugUtils::errored(kErrorInvalidSegment);

        // Validate AVX-512 broadcast {1tox}.
        if (m.hasBroadcast()) {
          if (memSize != 0) {
            // If the size is specified it has to match the broadcast size.
            if (ASMJIT_UNLIKELY(commonInfo.hasAvx512B32() && memSize != 4))
              return DebugUtils::errored(kErrorInvalidBroadcast);

            if (ASMJIT_UNLIKELY(commonInfo.hasAvx512B64() && memSize != 8))
              return DebugUtils::errored(kErrorInvalidBroadcast);
          }
          else {
            // If there is no size we implicitly calculate it so we can validate N in {1toN} properly.
            memSize = commonInfo.hasAvx512B32() ? 4 : 8;
          }

          memSize <<= m.getBroadcast();
        }

        if (baseType) {
          uint32_t baseId = m.baseId();

          if (m.isRegHome()) {
            // Home address of a virtual register. In such case we don't want to
            // validate the type of the base register as it will always be patched
            // to ESP|RSP.
          }
          else {
            if (ASMJIT_UNLIKELY((vd->allowedMemBaseRegs & (1u << baseType)) == 0))
              return DebugUtils::errored(kErrorInvalidAddress);
          }

          // Create information that will be validated only if this is an implicit
          // memory operand. Basically only usable for string instructions and other
          // instructions where memory operand is implicit and has 'seg:[reg]' form.
          if (baseId < Operand::kVirtIdMin) {
            // Physical base id.
            regMask = Support::bitMask(baseId);
            combinedRegMask |= regMask;
          }
          else {
            // Virtual base id - fill the whole mask for implicit mem validation.
            // The register is not assigned yet, so we cannot predict the phys id.
            regMask = 0xFFFFFFFFu;
          }

          if (!indexType && !m.offsetLo32())
            memFlags |= InstDB::kMemOpBaseOnly;
        }
        else {
          // Base is a 64-bit address.
          int64_t offset = m.offset();
          if (!Support::isInt32(offset)) {
            if (mode == InstDB::kModeX86) {
              // 32-bit mode: Make sure that the address is either `int32_t` or `uint32_t`.
              if (!Support::isUInt32(offset))
                return DebugUtils::errored(kErrorInvalidAddress64Bit);
            }
            else {
              // 64-bit mode: Zero extension is allowed if the address has 32-bit index
              // register or the address has no index register (it's still encodable).
              if (indexType) {
                if (!Support::isUInt32(offset))
                  return DebugUtils::errored(kErrorInvalidAddress64Bit);

                if (indexType != Reg::kTypeGpd)
                  return DebugUtils::errored(kErrorInvalidAddress64BitZeroExtension);
              }
              else {
                // We don't validate absolute 64-bit addresses without an index register
                // as this also depends on the target's base address. We don't have the
                // information to do it at this moment.
              }
            }
          }
        }

        if (indexType) {
          if (ASMJIT_UNLIKELY((vd->allowedMemIndexRegs & (1u << indexType)) == 0))
            return DebugUtils::errored(kErrorInvalidAddress);

          if (indexType == Reg::kTypeXmm) {
            opFlags |= InstDB::kOpVm;
            memFlags |= InstDB::kMemOpVm32x | InstDB::kMemOpVm64x;
          }
          else if (indexType == Reg::kTypeYmm) {
            opFlags |= InstDB::kOpVm;
            memFlags |= InstDB::kMemOpVm32y | InstDB::kMemOpVm64y;
          }
          else if (indexType == Reg::kTypeZmm) {
            opFlags |= InstDB::kOpVm;
            memFlags |= InstDB::kMemOpVm32z | InstDB::kMemOpVm64z;
          }
          else {
            opFlags |= InstDB::kOpMem;
            if (baseType)
              memFlags |= InstDB::kMemOpMib;
          }

          // [RIP + {XMM|YMM|ZMM}] is not allowed.
          if (baseType == Reg::kTypeRip && (opFlags & InstDB::kOpVm))
            return DebugUtils::errored(kErrorInvalidAddress);

          uint32_t indexId = m.indexId();
          if (indexId < Operand::kVirtIdMin)
            combinedRegMask |= Support::bitMask(indexId);

          // Only used for implicit memory operands having 'seg:[reg]' form, so clear it.
          regMask = 0;
        }
        else {
          opFlags |= InstDB::kOpMem;
        }

        switch (memSize) {
          case  0: memFlags |= InstDB::kMemOpAny ; break;
          case  1: memFlags |= InstDB::kMemOpM8  ; break;
          case  2: memFlags |= InstDB::kMemOpM16 ; break;
          case  4: memFlags |= InstDB::kMemOpM32 ; break;
          case  6: memFlags |= InstDB::kMemOpM48 ; break;
          case  8: memFlags |= InstDB::kMemOpM64 ; break;
          case 10: memFlags |= InstDB::kMemOpM80 ; break;
          case 16: memFlags |= InstDB::kMemOpM128; break;
          case 32: memFlags |= InstDB::kMemOpM256; break;
          case 64: memFlags |= InstDB::kMemOpM512; break;
          default:
            return DebugUtils::errored(kErrorInvalidOperandSize);
        }

        break;
      }

      case Operand::kOpImm: {
        uint64_t immValue = op.as<Imm>().u64();
        uint32_t immFlags = 0;

        if (int64_t(immValue) >= 0) {
          if (immValue <= 0x7u)
            immFlags = InstDB::kOpI64 | InstDB::kOpU64 | InstDB::kOpI32 | InstDB::kOpU32 |
                       InstDB::kOpI16 | InstDB::kOpU16 | InstDB::kOpI8  | InstDB::kOpU8  |
                       InstDB::kOpI4  | InstDB::kOpU4  ;
          else if (immValue <= 0xFu)
            immFlags = InstDB::kOpI64 | InstDB::kOpU64 | InstDB::kOpI32 | InstDB::kOpU32 |
                       InstDB::kOpI16 | InstDB::kOpU16 | InstDB::kOpI8  | InstDB::kOpU8  |
                       InstDB::kOpU4  ;
          else if (immValue <= 0x7Fu)
            immFlags = InstDB::kOpI64 | InstDB::kOpU64 | InstDB::kOpI32 | InstDB::kOpU32 |
                       InstDB::kOpI16 | InstDB::kOpU16 | InstDB::kOpI8  | InstDB::kOpU8  ;
          else if (immValue <= 0xFFu)
            immFlags = InstDB::kOpI64 | InstDB::kOpU64 | InstDB::kOpI32 | InstDB::kOpU32 |
                       InstDB::kOpI16 | InstDB::kOpU16 | InstDB::kOpU8  ;
          else if (immValue <= 0x7FFFu)
            immFlags = InstDB::kOpI64 | InstDB::kOpU64 | InstDB::kOpI32 | InstDB::kOpU32 |
                       InstDB::kOpI16 | InstDB::kOpU16 ;
          else if (immValue <= 0xFFFFu)
            immFlags = InstDB::kOpI64 | InstDB::kOpU64 | InstDB::kOpI32 | InstDB::kOpU32 |
                       InstDB::kOpU16 ;
          else if (immValue <= 0x7FFFFFFFu)
            immFlags = InstDB::kOpI64 | InstDB::kOpU64 | InstDB::kOpI32 | InstDB::kOpU32;
          else if (immValue <= 0xFFFFFFFFu)
            immFlags = InstDB::kOpI64 | InstDB::kOpU64 | InstDB::kOpU32;
          else if (immValue <= 0x7FFFFFFFFFFFFFFFu)
            immFlags = InstDB::kOpI64 | InstDB::kOpU64;
          else
            immFlags = InstDB::kOpU64;
        }
        else {
          immValue = Support::neg(immValue);
          if (immValue <= 0x8u)
            immFlags = InstDB::kOpI64 | InstDB::kOpI32 | InstDB::kOpI16 | InstDB::kOpI8 | InstDB::kOpI4;
          else if (immValue <= 0x80u)
            immFlags = InstDB::kOpI64 | InstDB::kOpI32 | InstDB::kOpI16 | InstDB::kOpI8;
          else if (immValue <= 0x8000u)
            immFlags = InstDB::kOpI64 | InstDB::kOpI32 | InstDB::kOpI16;
          else if (immValue <= 0x80000000u)
            immFlags = InstDB::kOpI64 | InstDB::kOpI32;
          else
            immFlags = InstDB::kOpI64;
        }
        opFlags |= immFlags;
        break;
      }

      case Operand::kOpLabel: {
        opFlags |= InstDB::kOpRel8 | InstDB::kOpRel32;
        break;
      }

      default:
        return DebugUtils::errored(kErrorInvalidState);
    }

    InstDB::OpSignature& oSigDst = oSigTranslated[i];
    oSigDst.opFlags = opFlags;
    oSigDst.memFlags = uint16_t(memFlags);
    oSigDst.regMask = uint8_t(regMask & 0xFFu);
    combinedOpFlags |= opFlags;
  }

  // Decrease the number of operands of those that are none. This is important
  // as Assembler and Compiler may just pass more operands padded with none
  // (which means that no operand is given at that index). However, validate
  // that there are no gaps (like [reg, none, reg] or [none, reg]).
  if (i < opCount) {
    while (--opCount > i)
      if (ASMJIT_UNLIKELY(!operands[opCount].isNone()))
        return DebugUtils::errored(kErrorInvalidInstruction);
  }

  // Validate X86 and X64 specific cases.
  if (mode == InstDB::kModeX86) {
    // Illegal use of 64-bit register in 32-bit mode.
    if (ASMJIT_UNLIKELY((combinedOpFlags & InstDB::kOpGpq) != 0))
      return DebugUtils::errored(kErrorInvalidUseOfGpq);
  }
  else {
    // Illegal use of a high 8-bit register with REX prefix.
    if (ASMJIT_UNLIKELY((combinedOpFlags & InstDB::kOpGpbHi) != 0 && (combinedRegMask & 0xFFFFFF00u) != 0))
      return DebugUtils::errored(kErrorInvalidUseOfGpbHi);
  }

  // --------------------------------------------------------------------------
  // [Validate Instruction Signature by Comparing Against All `iSig` Rows]
  // --------------------------------------------------------------------------

  const InstDB::InstSignature* iSig = InstDB::_instSignatureTable + commonInfo._iSignatureIndex;
  const InstDB::InstSignature* iEnd = iSig + commonInfo._iSignatureCount;

  if (iSig != iEnd) {
    const InstDB::OpSignature* opSignatureTable = InstDB::_opSignatureTable;

    // If set it means that we matched a signature where only immediate value
    // was out of bounds. We can return a more descriptive error if we know this.
    bool globalImmOutOfRange = false;

    do {
      // Check if the architecture is compatible.
      if ((iSig->modes & mode) == 0)
        continue;

      // Compare the operands table with reference operands.
      uint32_t j = 0;
      uint32_t iSigCount = iSig->opCount;
      bool localImmOutOfRange = false;

      if (iSigCount == opCount) {
        for (j = 0; j < opCount; j++)
          if (!x86CheckOSig(oSigTranslated[j], opSignatureTable[iSig->operands[j]], localImmOutOfRange))
            break;
      }
      else if (iSigCount - iSig->implicit == opCount) {
        uint32_t r = 0;
        for (j = 0; j < opCount && r < iSigCount; j++, r++) {
          const InstDB::OpSignature* oChk = oSigTranslated + j;
          const InstDB::OpSignature* oRef;
Next:
          oRef = opSignatureTable + iSig->operands[r];
          // Skip implicit.
          if ((oRef->opFlags & InstDB::kOpImplicit) != 0) {
            if (++r >= iSigCount)
              break;
            else
              goto Next;
          }

          if (!x86CheckOSig(*oChk, *oRef, localImmOutOfRange))
            break;
        }
      }

      if (j == opCount) {
        if (!localImmOutOfRange) {
          // Match, must clear possible `globalImmOutOfRange`.
          globalImmOutOfRange = false;
          break;
        }
        globalImmOutOfRange = localImmOutOfRange;
      }
    } while (++iSig != iEnd);

    if (iSig == iEnd) {
      if (globalImmOutOfRange)
        return DebugUtils::errored(kErrorInvalidImmediate);
      else
        return DebugUtils::errored(kErrorInvalidInstruction);
    }
  }

  // --------------------------------------------------------------------------
  // [Validate AVX512 Options]
  // --------------------------------------------------------------------------

  const RegOnly& extraReg = inst.extraReg();
  const uint32_t kAvx512Options = Inst::kOptionZMask   |
                                  Inst::kOptionER      |
                                  Inst::kOptionSAE     ;

  if (options & kAvx512Options) {
    if (commonInfo.hasFlag(InstDB::kFlagEvex)) {
      // Validate AVX-512 {z}.
      if ((options & Inst::kOptionZMask)) {
        if (ASMJIT_UNLIKELY((options & Inst::kOptionZMask) != 0 && !commonInfo.hasAvx512Z()))
          return DebugUtils::errored(kErrorInvalidKZeroUse);
      }

      // Validate AVX-512 {sae} and {er}.
      if (options & (Inst::kOptionSAE | Inst::kOptionER)) {
        // Rounding control is impossible if the instruction is not reg-to-reg.
        if (ASMJIT_UNLIKELY(memOp))
          return DebugUtils::errored(kErrorInvalidEROrSAE);

        // Check if {sae} or {er} is supported by the instruction.
        if (options & Inst::kOptionER) {
          // NOTE: if both {sae} and {er} are set, we don't care, as {sae} is implied.
          if (ASMJIT_UNLIKELY(!commonInfo.hasAvx512ER()))
            return DebugUtils::errored(kErrorInvalidEROrSAE);
        }
        else {
          if (ASMJIT_UNLIKELY(!commonInfo.hasAvx512SAE()))
            return DebugUtils::errored(kErrorInvalidEROrSAE);
        }

        // {sae} and {er} are defined for either scalar ops or vector ops that
        // require LL to be 10 (512-bit vector operations). We don't need any
        // more bits in the instruction database to be able to validate this, as
        // each AVX512 instruction that has broadcast is vector instruction (in
        // this case we require zmm registers), otherwise it's a scalar instruction,
        // which is valid.
        if (commonInfo.hasAvx512B()) {
          // Supports broadcast, thus we require LL to be '10', which means there
          // have to be ZMM registers used. We don't calculate LL here, but we know
          // that it would be '10' if there is at least one ZMM register used.

          // There is no {er}/{sae}-enabled instruction with less than two operands.
          ASMJIT_ASSERT(opCount >= 2);
          if (ASMJIT_UNLIKELY(!x86IsZmmOrM512(operands[0]) && !x86IsZmmOrM512(operands[1])))
            return DebugUtils::errored(kErrorInvalidEROrSAE);
        }
      }
    }
    else {
      // Not AVX512 instruction - maybe OpExtra is xCX register used by REP/REPNE
      // prefix. Otherwise the instruction is invalid.
      if ((options & kAvx512Options) || (options & kRepAny) == 0)
        return DebugUtils::errored(kErrorInvalidInstruction);
    }
  }

  // --------------------------------------------------------------------------
  // [Validate {Extra} Register]
  // --------------------------------------------------------------------------

  if (extraReg.isReg()) {
    if (options & kRepAny) {
      // Validate REP|REPNE {cx|ecx|rcx}.
      if (ASMJIT_UNLIKELY(iFlags & InstDB::kFlagRepIgnored))
        return DebugUtils::errored(kErrorInvalidExtraReg);

      if (extraReg.isPhysReg()) {
        if (ASMJIT_UNLIKELY(extraReg.id() != Gp::kIdCx))
          return DebugUtils::errored(kErrorInvalidExtraReg);
      }

      // The type of the {...} register must match the type of the base register
      // of memory operand. So if the memory operand uses 32-bit register the
      // count register must also be 32-bit, etc...
      if (ASMJIT_UNLIKELY(!memOp || extraReg.type() != memOp->baseType()))
        return DebugUtils::errored(kErrorInvalidExtraReg);
    }
    else if (commonInfo.hasFlag(InstDB::kFlagEvex)) {
      // Validate AVX-512 {k}.
      if (ASMJIT_UNLIKELY(extraReg.type() != Reg::kTypeKReg))
        return DebugUtils::errored(kErrorInvalidExtraReg);

      if (ASMJIT_UNLIKELY(extraReg.id() == 0 || !commonInfo.hasAvx512K()))
        return DebugUtils::errored(kErrorInvalidKMaskUse);
    }
    else {
      return DebugUtils::errored(kErrorInvalidExtraReg);
    }
  }

  return kErrorOk;
}
#endif // !ASMJIT_NO_VALIDATION

// ============================================================================
// [asmjit::x86::InstInternal - QueryRWInfo]
// ============================================================================

#ifndef ASMJIT_NO_INTROSPECTION
static const uint64_t rwRegGroupByteMask[Reg::kGroupCount] = {
  0x00000000000000FFu, // GP.
  0xFFFFFFFFFFFFFFFFu, // XMM|YMM|ZMM.
  0x00000000000000FFu, // MM.
  0x00000000000000FFu, // KReg.
  0x0000000000000003u, // SReg.
  0x00000000000000FFu, // CReg.
  0x00000000000000FFu, // DReg.
  0x00000000000003FFu, // St().
  0x000000000000FFFFu, // BND.
  0x00000000000000FFu  // RIP.
};

// TODO: Make universal.
static ASMJIT_INLINE uint32_t gpRegSizeByArchId(uint32_t archId) noexcept {
  static const uint8_t table[] = { 0, 4, 8, 4, 8 };
  return table[archId];
}

static ASMJIT_INLINE void rwZeroExtendGp(OpRWInfo& opRwInfo, const Gp& reg, uint32_t nativeGpSize) noexcept {
  ASMJIT_ASSERT(BaseReg::isGp(reg.as<Operand>()));
  if (reg.size() + 4 == nativeGpSize) {
    opRwInfo.addOpFlags(OpRWInfo::kZExt);
    opRwInfo.setExtendByteMask(~opRwInfo.writeByteMask() & 0xFFu);
  }
}

static ASMJIT_INLINE void rwZeroExtendAvxVec(OpRWInfo& opRwInfo, const Vec& reg) noexcept {
  ASMJIT_UNUSED(reg);

  uint64_t msk = ~Support::fillTrailingBits(opRwInfo.writeByteMask());
  if (msk) {
    opRwInfo.addOpFlags(OpRWInfo::kZExt);
    opRwInfo.setExtendByteMask(msk);
  }
}

static ASMJIT_INLINE void rwZeroExtendNonVec(OpRWInfo& opRwInfo, const Reg& reg) noexcept {
  uint64_t msk = ~Support::fillTrailingBits(opRwInfo.writeByteMask()) & rwRegGroupByteMask[reg.group()];
  if (msk) {
    opRwInfo.addOpFlags(OpRWInfo::kZExt);
    opRwInfo.setExtendByteMask(msk);
  }
}

Error InstInternal::queryRWInfo(uint32_t archId, const BaseInst& inst, const Operand_* operands, uint32_t opCount, InstRWInfo& out) noexcept {
  using namespace Status;

  // Only called when `archId` matches X86 family.
  ASMJIT_ASSERT(ArchInfo::isX86Family(archId));

  // Get the instruction data.
  uint32_t instId = inst.id();
  if (ASMJIT_UNLIKELY(!Inst::isDefinedId(instId)))
    return DebugUtils::errored(kErrorInvalidInstruction);

  // Read/Write flags.
  const InstDB::CommonInfoTableB& tabB = InstDB::_commonInfoTableB[InstDB::_instInfoTable[instId]._commonInfoIndexB];
  const InstDB::RWFlagsInfoTable& rwFlags = InstDB::_rwFlagsInfoTable[tabB._rwFlagsIndex];

  // Each RWInfo contains two indexes
  //   [0] - OpCount == 2
  //   [1] - OpCount != 2
  // They are used this way as there are instructions that have 2 and 3
  // operand overloads that use different semantics. So instead of adding
  // more special cases we just separated their data tables.
  const InstDB::RWInfo& instRwInfo = InstDB::rwInfo[InstDB::rwInfoIndex[instId * 2u + uint32_t(opCount != 2)]];
  const InstDB::RWInfoRm& instRmInfo = InstDB::rwInfoRm[instRwInfo.rmInfo];

  out._instFlags = 0;
  out._opCount = uint8_t(opCount);
  out._rmFeature = instRmInfo.rmFeature;
  out._extraReg.reset();
  out._readFlags = rwFlags.readFlags;
  out._writeFlags = rwFlags.writeFlags;

  uint32_t nativeGpSize = gpRegSizeByArchId(archId);

  constexpr uint32_t R = OpRWInfo::kRead;
  constexpr uint32_t W = OpRWInfo::kWrite;
  constexpr uint32_t X = OpRWInfo::kRW;
  constexpr uint32_t RegM = OpRWInfo::kRegMem;
  constexpr uint32_t RegPhys = OpRWInfo::kRegPhysId;
  constexpr uint32_t MibRead = OpRWInfo::kMemBaseRead | OpRWInfo::kMemIndexRead;

  if (ASMJIT_LIKELY(instRwInfo.category == InstDB::RWInfo::kCategoryGeneric)) {
    uint32_t i;
    uint32_t rmOpsMask = 0;
    uint32_t rmMaxSize = 0;

    for (i = 0; i < opCount; i++) {
      OpRWInfo& op = out._operands[i];
      const Operand_& srcOp = operands[i];
      const InstDB::RWInfoOp& rwOpData = InstDB::rwInfoOp[instRwInfo.opInfoIndex[i]];

      if (!srcOp.isRegOrMem()) {
        op.reset();
        continue;
      }

      op._opFlags = rwOpData.flags & ~(OpRWInfo::kZExt);
      op._physId = rwOpData.physId;
      op._rmSize = 0;
      op._resetReserved();

      uint64_t rByteMask = rwOpData.rByteMask;
      uint64_t wByteMask = rwOpData.wByteMask;

      if (op.isRead()  && !rByteMask) rByteMask = Support::lsbMask<uint64_t>(srcOp.size());
      if (op.isWrite() && !wByteMask) wByteMask = Support::lsbMask<uint64_t>(srcOp.size());

      op._readByteMask = rByteMask;
      op._writeByteMask = wByteMask;
      op._extendByteMask = 0;

      if (srcOp.isReg()) {
        // Zero extension.
        if (op.isWrite()) {
          if (srcOp.as<Reg>().isGp()) {
            // GP registers on X64 are special:
            //   - 8-bit and 16-bit writes aren't zero extended.
            //   - 32-bit writes ARE zero extended.
            rwZeroExtendGp(op, srcOp.as<Gp>(), nativeGpSize);
          }
          else if (rwOpData.flags & OpRWInfo::kZExt) {
            // Otherwise follow ZExt.
            rwZeroExtendNonVec(op, srcOp.as<Gp>());
          }
        }

        // Aggregate values required to calculate valid Reg/M info.
        rmMaxSize  = Support::max(rmMaxSize, srcOp.size());
        rmOpsMask |= Support::bitMask<uint32_t>(i);
      }
      else {
        op.addOpFlags(MibRead);
      }
    }

    rmOpsMask &= instRmInfo.rmOpsMask;
    if (rmOpsMask) {
      Support::BitWordIterator<uint32_t> it(rmOpsMask);
      do {
        i = it.next();

        OpRWInfo& op = out._operands[i];
        op.addOpFlags(RegM);

        switch (instRmInfo.category) {
          case InstDB::RWInfoRm::kCategoryFixed:
            op.setRmSize(instRmInfo.fixedSize);
            break;
          case InstDB::RWInfoRm::kCategoryConsistent:
            op.setRmSize(operands[i].size());
            break;
          case InstDB::RWInfoRm::kCategoryHalf:
            op.setRmSize(rmMaxSize / 2u);
            break;
          case InstDB::RWInfoRm::kCategoryQuarter:
            op.setRmSize(rmMaxSize / 4u);
            break;
          case InstDB::RWInfoRm::kCategoryEighth:
            op.setRmSize(rmMaxSize / 8u);
            break;
        }
      } while (it.hasNext());
    }

    return kErrorOk;
  }

  switch (instRwInfo.category) {
    case InstDB::RWInfo::kCategoryMov: {
      // Special case for 'movhpd' instruction. Here there are some variants that
      // we have to handle as mov can be used to move between GP, segment, control
      // and debug registers. Moving between GP registers also allow to use memory
      // operand.

      if (opCount == 2) {
        if (operands[0].isReg() && operands[1].isReg()) {
          const Reg& o0 = operands[0].as<Reg>();
          const Reg& o1 = operands[1].as<Reg>();

          if (o0.isGp() && o1.isGp()) {
            out._operands[0].reset(W | RegM, operands[0].size());
            out._operands[1].reset(R | RegM, operands[1].size());

            rwZeroExtendGp(out._operands[0], operands[0].as<Gp>(), nativeGpSize);
            return kErrorOk;
          }

          if (o0.isGp() && o1.isSReg()) {
            out._operands[0].reset(W | RegM, nativeGpSize);
            out._operands[0].setRmSize(2);
            out._operands[1].reset(R, 2);
            return kErrorOk;
          }

          if (o0.isSReg() && o1.isGp()) {
            out._operands[0].reset(W, 2);
            out._operands[1].reset(R | RegM, 2);
            out._operands[1].setRmSize(2);
            return kErrorOk;
          }

          if (o0.isGp() && (o1.isCReg() || o1.isDReg())) {
            out._operands[0].reset(W, nativeGpSize);
            out._operands[1].reset(R, nativeGpSize);
            out._writeFlags = kOF | kSF | kZF | kAF | kPF | kCF;
            return kErrorOk;
          }

          if ((o0.isCReg() || o0.isDReg()) && o1.isGp()) {
            out._operands[0].reset(W, nativeGpSize);
            out._operands[1].reset(R, nativeGpSize);
            out._writeFlags = kOF | kSF | kZF | kAF | kPF | kCF;
            return kErrorOk;
          }
        }

        if (operands[0].isReg() && operands[1].isMem()) {
          const Reg& o0 = operands[0].as<Reg>();
          const Mem& o1 = operands[1].as<Mem>();

          if (o0.isGp()) {
            if (!o1.isOffset64Bit())
              out._operands[0].reset(W, o0.size());
            else
              out._operands[0].reset(W | RegPhys, o0.size(), Gp::kIdAx);

            out._operands[1].reset(R | MibRead, o0.size());
            rwZeroExtendGp(out._operands[0], operands[0].as<Gp>(), nativeGpSize);
            return kErrorOk;
          }

          if (o0.isSReg()) {
            out._operands[0].reset(W, 2);
            out._operands[1].reset(R, 2);
            return kErrorOk;
          }
        }

        if (operands[0].isMem() && operands[1].isReg()) {
          const Mem& o0 = operands[0].as<Mem>();
          const Reg& o1 = operands[1].as<Reg>();

          if (o1.isGp()) {
            out._operands[0].reset(W | MibRead, o1.size());
            if (!o0.isOffset64Bit())
              out._operands[1].reset(R, o1.size());
            else
              out._operands[1].reset(R | RegPhys, o1.size(), Gp::kIdAx);
            return kErrorOk;
          }

          if (o1.isSReg()) {
            out._operands[0].reset(W | MibRead, 2);
            out._operands[1].reset(R, 2);
            return kErrorOk;
          }
        }

        if (Reg::isGp(operands[0]) && operands[1].isImm()) {
          const Reg& o0 = operands[0].as<Reg>();
          out._operands[0].reset(W | RegM, o0.size());
          out._operands[1].reset();

          rwZeroExtendGp(out._operands[0], operands[0].as<Gp>(), nativeGpSize);
          return kErrorOk;
        }

        if (operands[0].isMem() && operands[1].isImm()) {
          const Reg& o0 = operands[0].as<Reg>();
          out._operands[0].reset(W | MibRead, o0.size());
          out._operands[1].reset();
          return kErrorOk;
        }
      }
      break;
    }

    case InstDB::RWInfo::kCategoryImul: {
      // Special case for 'imul' instruction.
      //
      // There are 3 variants in general:
      //
      //   1. Standard multiplication: 'A = A * B'.
      //   2. Multiplication with imm: 'A = B * C'.
      //   3. Extended multiplication: 'A:B = B * C'.

      if (opCount == 2) {
        if (operands[0].isReg() && operands[1].isImm()) {
          out._operands[0].reset(X, operands[0].size());
          out._operands[1].reset();

          rwZeroExtendGp(out._operands[0], operands[0].as<Gp>(), nativeGpSize);
          return kErrorOk;
        }

        if (Reg::isGpw(operands[0]) && operands[1].size() == 1) {
          // imul ax, r8/m8 <- AX = AL * r8/m8
          out._operands[0].reset(X | RegPhys, 2, Gp::kIdAx);
          out._operands[0].setReadByteMask(Support::lsbMask<uint64_t>(1));
          out._operands[1].reset(R | RegM, 1);
        }
        else {
          // imul r?, r?/m?
          out._operands[0].reset(X, operands[0].size());
          out._operands[1].reset(R | RegM, operands[0].size());
          rwZeroExtendGp(out._operands[0], operands[0].as<Gp>(), nativeGpSize);
        }

        if (operands[1].isMem())
          out._operands[1].addOpFlags(MibRead);
        return kErrorOk;
      }

      if (opCount == 3) {
        if (operands[2].isImm()) {
          out._operands[0].reset(W, operands[0].size());
          out._operands[1].reset(R | RegM, operands[1].size());
          out._operands[2].reset();

          rwZeroExtendGp(out._operands[0], operands[0].as<Gp>(), nativeGpSize);
          if (operands[1].isMem())
            out._operands[1].addOpFlags(MibRead);
          return kErrorOk;
        }
        else {
          out._operands[0].reset(W | RegPhys, operands[0].size(), Gp::kIdDx);
          out._operands[1].reset(X | RegPhys, operands[1].size(), Gp::kIdAx);
          out._operands[2].reset(R | RegM, operands[2].size());

          rwZeroExtendGp(out._operands[0], operands[0].as<Gp>(), nativeGpSize);
          rwZeroExtendGp(out._operands[1], operands[1].as<Gp>(), nativeGpSize);
          if (operands[2].isMem())
            out._operands[2].addOpFlags(MibRead);
          return kErrorOk;
        }
      }
      break;
    }

    case InstDB::RWInfo::kCategoryMovh64: {
      // Special case for 'movhpd|movhps' instructions. Note that this is only
      // required for legacy (non-AVX) variants as AVX instructions use either
      // 2 or 3 operands that are use `kCategoryGeneric`.
      if (opCount == 2) {
        if (BaseReg::isVec(operands[0]) && operands[1].isMem()) {
          out._operands[0].reset(W, 8);
          out._operands[0].setWriteByteMask(Support::lsbMask<uint64_t>(8) << 8);
          out._operands[1].reset(R | MibRead, 8);
          return kErrorOk;
        }

        if (operands[0].isMem() && BaseReg::isVec(operands[1])) {
          out._operands[0].reset(W | MibRead, 8);
          out._operands[1].reset(R, 8);
          out._operands[1].setReadByteMask(Support::lsbMask<uint64_t>(8) << 8);
          return kErrorOk;
        }
      }
      break;
    }

    case InstDB::RWInfo::kCategoryVmaskmov: {
      // Special case for 'vmaskmovpd|vmaskmovps|vpmaskmovd|vpmaskmovq' instructions.
      if (opCount == 3) {
        if (BaseReg::isVec(operands[0]) && BaseReg::isVec(operands[1]) && operands[2].isMem()) {
          out._operands[0].reset(W, operands[0].size());
          out._operands[1].reset(R, operands[1].size());
          out._operands[2].reset(R | MibRead, operands[1].size());

          rwZeroExtendAvxVec(out._operands[0], operands[0].as<Vec>());
          return kErrorOk;
        }

        if (operands[0].isMem() && BaseReg::isVec(operands[1]) && BaseReg::isVec(operands[2])) {
          out._operands[0].reset(X | MibRead, operands[1].size());
          out._operands[1].reset(R, operands[1].size());
          out._operands[2].reset(R, operands[2].size());
          return kErrorOk;
        }
      }
      break;
    }

    case InstDB::RWInfo::kCategoryVmovddup: {
      // Special case for 'vmovddup' instruction. This instruction has an
      // interesting semantic as 128-bit XMM version only uses 64-bit memory
      // operand (m64), however, 256/512-bit versions use 256/512-bit memory
      // operand, respectively.
      if (opCount == 2) {
        if (BaseReg::isVec(operands[0]) && BaseReg::isVec(operands[1])) {
          uint32_t o0Size = operands[0].size();
          uint32_t o1Size = o0Size == 16 ? 8 : o0Size;

          out._operands[0].reset(W, o0Size);
          out._operands[1].reset(R | RegM, o1Size);
          out._operands[1]._readByteMask &= 0x00FF00FF00FF00FFu;

          rwZeroExtendAvxVec(out._operands[0], operands[0].as<Vec>());
          return kErrorOk;
        }

        if (BaseReg::isVec(operands[0]) && operands[1].isMem()) {
          uint32_t o0Size = operands[0].size();
          uint32_t o1Size = o0Size == 16 ? 8 : o0Size;

          out._operands[0].reset(W, o0Size);
          out._operands[1].reset(R | MibRead, o1Size);

          rwZeroExtendAvxVec(out._operands[0], operands[0].as<Vec>());
          return kErrorOk;
        }
      }
      break;
    }

    case InstDB::RWInfo::kCategoryVmovmskpd:
    case InstDB::RWInfo::kCategoryVmovmskps: {
      // Special case for 'vmovmskpd|vmovmskps' instructions.
      if (opCount == 2) {
        if (BaseReg::isGp(operands[0]) && BaseReg::isVec(operands[1])) {
          out._operands[0].reset(W, 1);
          out._operands[0].setExtendByteMask(Support::lsbMask<uint32_t>(nativeGpSize - 1) << 1);
          out._operands[1].reset(R, operands[1].size());
          return kErrorOk;
        }
      }
      break;
    }

    case InstDB::RWInfo::kCategoryVmov1_2:
    case InstDB::RWInfo::kCategoryVmov1_4:
    case InstDB::RWInfo::kCategoryVmov1_8: {
      // Special case for instructions where the destination is 1:N (narrowing).
      //
      // Vmov1_2:
      //   vcvtpd2dq|vcvttpd2dq
      //   vcvtpd2udq|vcvttpd2udq
      //   vcvtpd2ps|vcvtps2ph
      //   vcvtqq2ps|vcvtuqq2ps
      //   vpmovwb|vpmovswb|vpmovuswb
      //   vpmovdw|vpmovsdw|vpmovusdw
      //   vpmovqd|vpmovsqd|vpmovusqd
      //
      // Vmov1_4:
      //   vpmovdb|vpmovsdb|vpmovusdb
      //   vpmovqw|vpmovsqw|vpmovusqw
      //
      // Vmov1_8:
      //   pmovmskb|vpmovmskb
      //   vpmovqb|vpmovsqb|vpmovusqb
      uint32_t shift = instRwInfo.category - InstDB::RWInfo::kCategoryVmov1_2 + 1;

      if (opCount >= 2) {
        if (opCount >= 3) {
          if (opCount > 3)
            return DebugUtils::errored(kErrorInvalidInstruction);
          out._operands[2].reset();
        }

        if (operands[0].isReg() && operands[1].isReg()) {
          uint32_t size1 = operands[1].size();
          uint32_t size0 = size1 >> shift;

          out._operands[0].reset(W, size0);
          out._operands[1].reset(R, size1);

          if (instRmInfo.rmOpsMask & 0x1) {
            out._operands[0].addOpFlags(RegM);
            out._operands[0].setRmSize(size0);
          }

          if (instRmInfo.rmOpsMask & 0x2) {
            out._operands[1].addOpFlags(RegM);
            out._operands[1].setRmSize(size1);
          }

          // Handle 'pmovmskb|vpmovmskb'.
          if (BaseReg::isGp(operands[0]))
            rwZeroExtendGp(out._operands[0], operands[0].as<Gp>(), nativeGpSize);

          if (BaseReg::isVec(operands[0]))
            rwZeroExtendAvxVec(out._operands[0], operands[0].as<Vec>());

          return kErrorOk;
        }

        if (operands[0].isReg() && operands[1].isMem()) {
          uint32_t size1 = operands[1].size() ? operands[1].size() : uint32_t(16);
          uint32_t size0 = size1 >> shift;

          out._operands[0].reset(W, size0);
          out._operands[1].reset(R | MibRead, size1);
          return kErrorOk;
        }

        if (operands[0].isMem() && operands[1].isReg()) {
          uint32_t size1 = operands[1].size();
          uint32_t size0 = size1 >> shift;

          out._operands[0].reset(W | MibRead, size0);
          out._operands[1].reset(R, size1);
          return kErrorOk;
        }
      }
      break;
    }

    case InstDB::RWInfo::kCategoryVmov2_1:
    case InstDB::RWInfo::kCategoryVmov4_1:
    case InstDB::RWInfo::kCategoryVmov8_1: {
      // Special case for instructions where the destination is N:1 (widening).
      //
      // Vmov2_1:
      //   vcvtdq2pd|vcvtudq2pd
      //   vcvtps2pd|vcvtph2ps
      //   vcvtps2qq|vcvtps2uqq
      //   vcvttps2qq|vcvttps2uqq
      //   vpmovsxbw|vpmovzxbw
      //   vpmovsxwd|vpmovzxwd
      //   vpmovsxdq|vpmovzxdq
      //
      // Vmov4_1:
      //   vpmovsxbd|vpmovzxbd
      //   vpmovsxwq|vpmovzxwq
      //
      // Vmov8_1:
      //   vpmovsxbq|vpmovzxbq
      uint32_t shift = instRwInfo.category - InstDB::RWInfo::kCategoryVmov2_1 + 1;

      if (opCount >= 2) {
        if (opCount >= 3) {
          if (opCount > 3)
            return DebugUtils::errored(kErrorInvalidInstruction);
          out._operands[2].reset();
        }

        uint32_t size0 = operands[0].size();
        uint32_t size1 = size0 >> shift;

        out._operands[0].reset(W, size0);
        out._operands[1].reset(R, size1);

        if (operands[0].isReg() && operands[1].isReg()) {
          if (instRmInfo.rmOpsMask & 0x1) {
            out._operands[0].addOpFlags(RegM);
            out._operands[0].setRmSize(size0);
          }

          if (instRmInfo.rmOpsMask & 0x2) {
            out._operands[1].addOpFlags(RegM);
            out._operands[1].setRmSize(size1);
          }
          return kErrorOk;
        }

        if (operands[0].isReg() && operands[1].isMem()) {
          out._operands[1].addOpFlags(MibRead);
          return kErrorOk;
        }
      }
      break;
    }
  }

  return DebugUtils::errored(kErrorInvalidInstruction);
}
#endif // !ASMJIT_NO_INTROSPECTION

// ============================================================================
// [asmjit::x86::InstInternal - QueryFeatures]
// ============================================================================

#ifndef ASMJIT_NO_INTROSPECTION
struct RegAnalysis {
  uint32_t regTypeMask;
  uint32_t highVecUsed;

  inline bool hasRegType(uint32_t regType) const noexcept {
    return Support::bitTest(regTypeMask, regType);
  }
};

static RegAnalysis InstInternal_regAnalysis(const Operand_* operands, uint32_t opCount) noexcept {
  uint32_t mask = 0;
  uint32_t highVecUsed = 0;

  for (uint32_t i = 0; i < opCount; i++) {
    const Operand_& op = operands[i];
    if (op.isReg()) {
      const BaseReg& reg = op.as<BaseReg>();
      mask |= Support::bitMask(reg.type());
      if (reg.isVec())
        highVecUsed |= uint32_t(reg.id() >= 16 && reg.id() < 32);
    }
    else if (op.isMem()) {
      const BaseMem& mem = op.as<BaseMem>();
      if (mem.hasBaseReg()) mask |= Support::bitMask(mem.baseType());
      if (mem.hasIndexReg()) {
        mask |= Support::bitMask(mem.indexType());
        highVecUsed |= uint32_t(mem.indexId() >= 16 && mem.indexId() < 32);
      }
    }
  }

  return RegAnalysis { mask, highVecUsed };
}

Error InstInternal::queryFeatures(uint32_t archId, const BaseInst& inst, const Operand_* operands, uint32_t opCount, BaseFeatures& out) noexcept {
  // Only called when `archId` matches X86 family.
  ASMJIT_UNUSED(archId);
  ASMJIT_ASSERT(ArchInfo::isX86Family(archId));

  // Get the instruction data.
  uint32_t instId = inst.id();
  uint32_t options = inst.options();

  if (ASMJIT_UNLIKELY(!Inst::isDefinedId(instId)))
    return DebugUtils::errored(kErrorInvalidInstruction);

  const InstDB::InstInfo& instInfo = InstDB::infoById(instId);
  const InstDB::CommonInfoTableB& tableB = InstDB::_commonInfoTableB[instInfo._commonInfoIndexB];

  const uint8_t* fData = tableB.featuresBegin();
  const uint8_t* fEnd = tableB.featuresEnd();

  // Copy all features to `out`.
  out.reset();
  do {
    uint32_t feature = fData[0];
    if (!feature)
      break;
    out.add(feature);
  } while (++fData != fEnd);

  // Since AsmJit aggregates instructions that share the same name we have to
  // deal with some special cases and also with MMX/SSE and AVX/AVX2 overlaps.
  if (fData != tableB.featuresBegin()) {
    RegAnalysis regAnalysis = InstInternal_regAnalysis(operands, opCount);

    // Handle MMX vs SSE overlap.
    if (out.has(Features::kMMX) || out.has(Features::kMMX2)) {
      // Only instructions defined by SSE and SSE2 overlap. Instructions
      // introduced by newer instruction sets like SSE3+ don't state MMX as
      // they require SSE3+.
      if (out.has(Features::kSSE) || out.has(Features::kSSE2)) {
        if (!regAnalysis.hasRegType(Reg::kTypeXmm)) {
          // The instruction doesn't use XMM register(s), thus it's MMX/MMX2 only.
          out.remove(Features::kSSE);
          out.remove(Features::kSSE2);
        }
        else {
          out.remove(Features::kMMX);
          out.remove(Features::kMMX2);
        }

        // Special case: PEXTRW instruction is MMX/SSE2 instruction. However,
        // MMX/SSE version cannot access memory (only register to register
        // extract) so when SSE4.1 introduced the whole family of PEXTR/PINSR
        // instructions they also introduced PEXTRW with a new opcode 0x15 that
        // can extract directly to memory. This instruction is, of course, not
        // compatible with MMX/SSE2 and would #UD if SSE4.1 is not supported.
        if (instId == Inst::kIdPextrw) {
          ASMJIT_ASSERT(out.has(Features::kSSE2));
          ASMJIT_ASSERT(out.has(Features::kSSE4_1));

          if (opCount >= 1 && operands[0].isMem())
            out.remove(Features::kSSE2);
          else
            out.remove(Features::kSSE4_1);
        }
      }
    }

    // Handle PCLMULQDQ vs VPCLMULQDQ.
    if (out.has(Features::kVPCLMULQDQ)) {
      if (regAnalysis.hasRegType(Reg::kTypeZmm) || Support::bitTest(options, Inst::kOptionEvex)) {
        // AVX512_F & VPCLMULQDQ.
        out.remove(Features::kAVX, Features::kPCLMULQDQ);
      }
      else if (regAnalysis.hasRegType(Reg::kTypeYmm)) {
        out.remove(Features::kAVX512_F, Features::kAVX512_VL);
      }
      else {
        // AVX & PCLMULQDQ.
        out.remove(Features::kAVX512_F, Features::kAVX512_VL, Features::kVPCLMULQDQ);
      }
    }

    // Handle AVX vs AVX2 overlap.
    if (out.has(Features::kAVX) && out.has(Features::kAVX2)) {
      bool isAVX2 = true;
      // Special case: VBROADCASTSS and VBROADCASTSD were introduced in AVX, but
      // only version that uses memory as a source operand. AVX2 then added support
      // for register source operand.
      if (instId == Inst::kIdVbroadcastss || instId == Inst::kIdVbroadcastsd) {
        if (opCount > 1 && operands[1].isMem())
          isAVX2 = false;
      }
      else {
        // AVX instruction set doesn't support integer operations on YMM registers
        // as these were later introcuced by AVX2. In our case we have to check if
        // YMM register(s) are in use and if that is the case this is an AVX2 instruction.
        if (!(regAnalysis.regTypeMask & Support::bitMask(Reg::kTypeYmm, Reg::kTypeZmm)))
          isAVX2 = false;
      }

      if (isAVX2)
        out.remove(Features::kAVX);
      else
        out.remove(Features::kAVX2);
    }

    // Handle AVX|AVX2|FMA|F16C vs AVX512 overlap.
    if (out.has(Features::kAVX) || out.has(Features::kAVX2) || out.has(Features::kFMA) || out.has(Features::kF16C)) {
      // Only AVX512-F|BW|DQ allow to encode AVX/AVX2/FMA/F16C instructions
      if (out.has(Features::kAVX512_F) || out.has(Features::kAVX512_BW) || out.has(Features::kAVX512_DQ)) {
        uint32_t hasEvex = options & (Inst::kOptionEvex | Inst::_kOptionAvx512Mask);
        uint32_t hasKMask = inst.extraReg().type() == Reg::kTypeKReg;
        uint32_t hasKOrZmm = regAnalysis.regTypeMask & Support::bitMask(Reg::kTypeZmm, Reg::kTypeKReg);

        // Special case: VPSLLDQ and VPSRLDQ instructions only allow `reg, reg. imm`
        // combination in AVX|AVX2 mode, then AVX-512 introduced `reg, reg/mem, imm`
        // combination that uses EVEX prefix. This means that if the second operand
        // is memory then this is AVX-512_BW instruction and not AVX/AVX2 instruction.
        uint32_t mustUseEvex = (instId == Inst::kIdVpslldq || instId == Inst::kIdVpsrldq) && opCount >= 2 && operands[1].isMem();

        if (!(hasEvex | mustUseEvex | hasKMask | hasKOrZmm | regAnalysis.highVecUsed))
          out.remove(Features::kAVX512_F, Features::kAVX512_BW, Features::kAVX512_DQ, Features::kAVX512_VL);
        else
          out.remove(Features::kAVX, Features::kAVX2, Features::kFMA, Features::kF16C);
      }
    }

    // Clear AVX512_VL if ZMM register is used.
    if (regAnalysis.hasRegType(Reg::kTypeZmm))
      out.remove(Features::kAVX512_VL);
  }

  return kErrorOk;
}
#endif // !ASMJIT_NO_INTROSPECTION

// ============================================================================
// [asmjit::x86::InstInternal - Unit]
// ============================================================================

#if defined(ASMJIT_TEST)
UNIT(asmjit_x86_inst_api_text) {
  // All known instructions should be matched.
  INFO("Matching all X86 instructions");
  for (uint32_t a = 1; a < Inst::_kIdCount; a++) {
    StringTmp<128> aName;
    EXPECT(InstInternal::instIdToString(0, a, aName) == kErrorOk,
           "Failed to get the name of instruction #%u", a);

    uint32_t b = InstInternal::stringToInstId(0, aName.data(), aName.size());
    StringTmp<128> bName;
    InstInternal::instIdToString(0, b, bName);

    EXPECT(a == b,
           "Instructions do not match \"%s\" (#%u) != \"%s\" (#%u)", aName.data(), a, bName.data(), b);
  }
}
#endif

ASMJIT_END_SUB_NAMESPACE

#endif // ASMJIT_BUILD_X86
