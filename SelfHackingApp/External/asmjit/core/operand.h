// [AsmJit]
// Machine Code Generation for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

#ifndef _ASMJIT_CORE_OPERAND_H
#define _ASMJIT_CORE_OPERAND_H

#include "../core/support.h"

ASMJIT_BEGIN_NAMESPACE

// ============================================================================
// [Macros]
// ============================================================================

//! Adds a template specialization for `REG_TYPE` into the local `RegTraits`.
#define ASMJIT_DEFINE_REG_TRAITS(REG, REG_TYPE, GROUP, SIZE, COUNT, TYPE_ID)  \
template<>                                                                    \
struct RegTraits<REG_TYPE> {                                                  \
  typedef REG RegT;                                                           \
                                                                              \
  static constexpr uint32_t kValid = 1;                                       \
  static constexpr uint32_t kCount = COUNT;                                   \
  static constexpr uint32_t kTypeId = TYPE_ID;                                \
                                                                              \
  static constexpr uint32_t kType = REG_TYPE;                                 \
  static constexpr uint32_t kGroup = GROUP;                                   \
  static constexpr uint32_t kSize = SIZE;                                     \
                                                                              \
  static constexpr uint32_t kSignature =                                      \
    (Operand::kOpReg << Operand::kSignatureOpShift      ) |                   \
    (kType           << Operand::kSignatureRegTypeShift ) |                   \
    (kGroup          << Operand::kSignatureRegGroupShift) |                   \
    (kSize           << Operand::kSignatureSizeShift    ) ;                   \
}

//! Adds constructors and member functions to a class that implements abstract
//! register. Abstract register is register that doesn't have type or signature
//! yet, it's a base class like `x86::Reg` or `arm::Reg`.
#define ASMJIT_DEFINE_ABSTRACT_REG(REG, BASE)                                 \
public:                                                                       \
  /*! Default constructor that only setups basics. */                         \
  constexpr REG() noexcept                                                    \
    : BASE(kSignature, kIdBad) {}                                             \
                                                                              \
  /*! Makes a copy of the `other` register operand. */                        \
  constexpr REG(const REG& other) noexcept                                    \
    : BASE(other) {}                                                          \
                                                                              \
  /*! Makes a copy of the `other` register having id set to `rId` */          \
  constexpr REG(const BaseReg& other, uint32_t rId) noexcept                  \
    : BASE(other, rId) {}                                                     \
                                                                              \
  /*! Creates a register based on `signature` and `rId`. */                   \
  constexpr REG(uint32_t signature, uint32_t rId) noexcept                    \
    : BASE(signature, rId) {}                                                 \
                                                                              \
  /*! Creates a completely uninitialized REG register operand (garbage). */   \
  inline explicit REG(Globals::NoInit_) noexcept                              \
    : BASE(Globals::NoInit) {}                                                \
                                                                              \
  /*! Creates a new register from register type and id. */                    \
  static inline REG fromTypeAndId(uint32_t rType, uint32_t rId) noexcept {    \
    return REG(signatureOf(rType), rId);                                      \
  }                                                                           \
                                                                              \
  /*! Clones the register operand. */                                         \
  constexpr REG clone() const noexcept { return REG(*this); }                 \
                                                                              \
  inline REG& operator=(const REG& other) noexcept = default;

//! Adds constructors and member functions to a class that implements final
//! register. Final registers MUST HAVE a valid signature.
#define ASMJIT_DEFINE_FINAL_REG(REG, BASE, TRAITS)                            \
public:                                                                       \
  static constexpr uint32_t kThisType  = TRAITS::kType;                       \
  static constexpr uint32_t kThisGroup = TRAITS::kGroup;                      \
  static constexpr uint32_t kThisSize  = TRAITS::kSize;                       \
  static constexpr uint32_t kSignature = TRAITS::kSignature;                  \
                                                                              \
  ASMJIT_DEFINE_ABSTRACT_REG(REG, BASE)                                       \
                                                                              \
  /*! Creates a register operand having its id set to `rId`. */               \
  constexpr explicit REG(uint32_t rId) noexcept                               \
    : BASE(kSignature, rId) {}

//! \addtogroup asmjit_core
//! \{

// ============================================================================
// [asmjit::Operand_]
// ============================================================================

//! Constructor-less `Operand`.
//!
//! Contains no initialization code and can be used safely to define an array
//! of operands that won't be initialized. This is an `Operand` compatible
//! data structure designed to be statically initialized, static const, or to
//! be used by the user to define an array of operands without having them
//! default initialized.
//!
//! The key difference between `Operand` and `Operand_`:
//!
//! ```
//! Operand_ xArray[10]; // Not initialized, contains garbage.
//! Operand  yArray[10]; // All operands initialized to none.
//! ```
struct Operand_ {
  //! Operand's signature that provides operand type and additional information.
  uint32_t _signature;
  //! Either base id as used by memory operand or any id as used by others.
  uint32_t _baseId;

  //! Memory operand data.
  struct MemData {
    //! Index register id.
    uint32_t indexId;
    //! Low part of 64-bit offset (or 32-bit offset).
    uint32_t offsetLo32;
  };

  //! Additional data used by some operands.
  union {
    //! 32-bit data (used either by immediate or as a 32-bit view).
    uint32_t _data32[2];
    //! 64-bit data (used either by immediate or as a 64-bit view).
    uint64_t _data64;

    //! Memory address data.
    MemData _mem;
  };

  //! Operand types that can be encoded in `Operand`.
  enum OpType : uint32_t {
    //! Not an operand or not initialized.
    kOpNone = 0,
    //! Operand is a register.
    kOpReg = 1,
    //! Operand is a memory.
    kOpMem = 2,
    //! Operand is an immediate value.
    kOpImm = 3,
    //! Operand is a label.
    kOpLabel = 4
  };
  static_assert(kOpMem == kOpReg + 1, "asmjit::Operand requires `kOpMem` to be `kOpReg+1`.");

  // \cond INTERNAL
  enum SignatureBits : uint32_t {
    // Operand type (3 least significant bits).
    // |........|........|........|.....XXX|
    kSignatureOpShift = 0,
    kSignatureOpMask = 0x07u << kSignatureOpShift,

    // Register type (5 bits).
    // |........|........|........|XXXXX...|
    kSignatureRegTypeShift = 3,
    kSignatureRegTypeMask = 0x1Fu << kSignatureRegTypeShift,

    // Register group (4 bits).
    // |........|........|....XXXX|........|
    kSignatureRegGroupShift = 8,
    kSignatureRegGroupMask = 0x0Fu << kSignatureRegGroupShift,

    // Memory base type (5 bits).
    // |........|........|........|XXXXX...|
    kSignatureMemBaseTypeShift = 3,
    kSignatureMemBaseTypeMask = 0x1Fu << kSignatureMemBaseTypeShift,

    // Memory index type (5 bits).
    // |........|........|...XXXXX|........|
    kSignatureMemIndexTypeShift = 8,
    kSignatureMemIndexTypeMask = 0x1Fu << kSignatureMemIndexTypeShift,

    // Memory base+index combined (10 bits).
    // |........|........|...XXXXX|XXXXX...|
    kSignatureMemBaseIndexShift = 3,
    kSignatureMemBaseIndexMask = 0x3FFu << kSignatureMemBaseIndexShift,

    // Memory address type (2 bits).
    // |........|........|.XX.....|........|
    kSignatureMemAddrTypeShift = 13,
    kSignatureMemAddrTypeMask = 0x03u << kSignatureMemAddrTypeShift,

    // This memory operand represents a home-slot or stack (BaseCompiler).
    // |........|........|X.......|........|
    kSignatureMemRegHomeShift = 15,
    kSignatureMemRegHomeFlag = 0x01u << kSignatureMemRegHomeShift,

    // Operand size (8 most significant bits).
    // |XXXXXXXX|........|........|........|
    kSignatureSizeShift = 24,
    kSignatureSizeMask = 0xFFu << kSignatureSizeShift
  };
  //! \endcond

  //! \cond INTERNAL
  //! Constants useful for VirtId <-> Index translation.
  enum VirtIdConstants : uint32_t {
    //! Minimum valid packed-id.
    kVirtIdMin = 256,
    //! Maximum valid packed-id, excludes Globals::kInvalidId.
    kVirtIdMax = Globals::kInvalidId - 1,
    //! Count of valid packed-ids.
    kVirtIdCount = uint32_t(kVirtIdMax - kVirtIdMin + 1)
  };

  //! Tests whether the given `id` is a valid virtual register id. Since AsmJit
  //! supports both physical and virtual registers it must be able to distinguish
  //! between these two. The idea is that physical registers are always limited
  //! in size, so virtual identifiers start from `kVirtIdMin` and end at
  //! `kVirtIdMax`.
  static ASMJIT_INLINE bool isVirtId(uint32_t id) noexcept { return id - kVirtIdMin < uint32_t(kVirtIdCount); }
  //! Converts a real-id into a packed-id that can be stored in Operand.
  static ASMJIT_INLINE uint32_t indexToVirtId(uint32_t id) noexcept { return id + kVirtIdMin; }
  //! Converts a packed-id back to real-id.
  static ASMJIT_INLINE uint32_t virtIdToIndex(uint32_t id) noexcept { return id - kVirtIdMin; }
  //! \endcond

  //! \name Construction & Destruction
  //! \{

  //! \cond INTERNAL
  //! Initializes a `BaseReg` operand from `signature` and register `id`.
  inline void _initReg(uint32_t signature, uint32_t id) noexcept {
    _signature = signature;
    _baseId = id;
    _data64 = 0;
  }

  //! Initializes the operand from `other` (used by operator overloads).
  inline void copyFrom(const Operand_& other) noexcept { memcpy(this, &other, sizeof(Operand_)); }
  //! \endcond

  //! Resets the `Operand` to none.
  //!
  //! None operand is defined the following way:
  //!   - Its signature is zero (kOpNone, and the rest zero as well).
  //!   - Its id is `0`.
  //!   - The reserved8_4 field is set to `0`.
  //!   - The reserved12_4 field is set to zero.
  //!
  //! In other words, reset operands have all members set to zero. Reset operand
  //! must match the Operand state right after its construction. Alternatively,
  //! if you have an array of operands, you can simply use `memset()`.
  //!
  //! ```
  //! using namespace asmjit;
  //!
  //! Operand a;
  //! Operand b;
  //! assert(a == b);
  //!
  //! b = x86::eax;
  //! assert(a != b);
  //!
  //! b.reset();
  //! assert(a == b);
  //!
  //! memset(&b, 0, sizeof(Operand));
  //! assert(a == b);
  //! ```
  inline void reset() noexcept {
    _signature = 0;
    _baseId = 0;
    _data64 = 0;
  }

  //! \}

  //! \name Operator Overloads
  //! \{

  constexpr bool operator==(const Operand_& other) const noexcept { return  isEqual(other); }
  constexpr bool operator!=(const Operand_& other) const noexcept { return !isEqual(other); }

  //! \}

  //! \name Cast
  //! \{

  //! Casts this operand to `T` type.
  template<typename T>
  inline T& as() noexcept { return static_cast<T&>(*this); }

  //! Casts this operand to `T` type (const).
  template<typename T>
  inline const T& as() const noexcept { return static_cast<const T&>(*this); }

  //! \}

  //! \name Accessors
  //! \{

  //! Tests whether the operand matches the given signature `sign`.
  constexpr bool hasSignature(uint32_t signature) const noexcept { return _signature == signature; }
  //! Tests whether the operand matches the signature of the `other` operand.
  constexpr bool hasSignature(const Operand_& other) const noexcept { return _signature == other.signature(); }

  //! Returns operand signature as unsigned 32-bit integer.
  //!
  //! Signature is first 4 bytes of the operand data. It's used mostly for
  //! operand checking as it's much faster to check 4 bytes at once than having
  //! to check these bytes individually.
  constexpr uint32_t signature() const noexcept { return _signature; }

  //! Sets the operand signature, see `signature()`.
  //!
  //! \note Improper use of `setSignature()` can lead to hard-to-debug errors.
  inline void setSignature(uint32_t signature) noexcept { _signature = signature; }

  //! \cond INTERNAL
  template<uint32_t mask>
  constexpr bool _hasSignaturePart() const noexcept {
    return (_signature & mask) != 0;
  }

  template<uint32_t mask>
  constexpr uint32_t _getSignaturePart() const noexcept {
    return (_signature >> Support::constCtz(mask)) & (mask >> Support::constCtz(mask));
  }

  template<uint32_t mask>
  inline void _setSignaturePart(uint32_t value) noexcept {
    ASMJIT_ASSERT((value & ~(mask >> Support::constCtz(mask))) == 0);
    _signature = (_signature & ~mask) | (value << Support::constCtz(mask));
  }
  //! \endcond

  //! Returns the type of the operand, see `OpType`.
  constexpr uint32_t opType() const noexcept { return _getSignaturePart<kSignatureOpMask>(); }
  //! Tests whether the operand is none (`kOpNone`).
  constexpr bool isNone() const noexcept { return _signature == 0; }
  //! Tests whether the operand is a register (`kOpReg`).
  constexpr bool isReg() const noexcept { return opType() == kOpReg; }
  //! Tests whether the operand is a memory location (`kOpMem`).
  constexpr bool isMem() const noexcept { return opType() == kOpMem; }
  //! Tests whether the operand is an immediate (`kOpImm`).
  constexpr bool isImm() const noexcept { return opType() == kOpImm; }
  //! Tests whether the operand is a label (`kOpLabel`).
  constexpr bool isLabel() const noexcept { return opType() == kOpLabel; }

  //! Tests whether the operand is a physical register.
  constexpr bool isPhysReg() const noexcept { return isReg() && _baseId < 0xFFu; }
  //! Tests whether the operand is a virtual register.
  constexpr bool isVirtReg() const noexcept { return isReg() && _baseId > 0xFFu; }

  //! Tests whether the operand specifies a size (i.e. the size is not zero).
  constexpr bool hasSize() const noexcept { return _hasSignaturePart<kSignatureSizeMask>(); }
  //! Tests whether the size of the operand matches `size`.
  constexpr bool hasSize(uint32_t s) const noexcept { return size() == s; }

  //! Returns the size of the operand in bytes.
  //!
  //! The value returned depends on the operand type:
  //!   * None  - Should always return zero size.
  //!   * Reg   - Should always return the size of the register. If the register
  //!             size depends on architecture (like `x86::CReg` and `x86::DReg`)
  //!             the size returned should be the greatest possible (so it should
  //!             return 64-bit size in such case).
  //!   * Mem   - Size is optional and will be in most cases zero.
  //!   * Imm   - Should always return zero size.
  //!   * Label - Should always return zero size.
  constexpr uint32_t size() const noexcept { return _getSignaturePart<kSignatureSizeMask>(); }

  //! Returns the operand id.
  //!
  //! The value returned should be interpreted accordingly to the operand type:
  //!   * None  - Should be `0`.
  //!   * Reg   - Physical or virtual register id.
  //!   * Mem   - Multiple meanings - BASE address (register or label id), or
  //!             high value of a 64-bit absolute address.
  //!   * Imm   - Should be `0`.
  //!   * Label - Label id if it was created by using `newLabel()` or
  //!             `Globals::kInvalidId` if the label is invalid or not
  //!             initialized.
  constexpr uint32_t id() const noexcept { return _baseId; }

  //! Tests whether the operand is 100% equal to `other`.
  constexpr bool isEqual(const Operand_& other) const noexcept {
    return (_signature == other._signature) &
           (_baseId    == other._baseId   ) &
           (_data64    == other._data64   ) ;
  }

  //! Tests whether the operand is a register matching `rType`.
  constexpr bool isReg(uint32_t rType) const noexcept {
    return (_signature & (kSignatureOpMask | kSignatureRegTypeMask)) ==
           ((kOpReg << kSignatureOpShift) | (rType << kSignatureRegTypeShift));
  }

  //! Tests whether the operand is register and of `rType` and `rId`.
  constexpr bool isReg(uint32_t rType, uint32_t rId) const noexcept {
    return isReg(rType) && id() == rId;
  }

  //! Tests whether the operand is a register or memory.
  constexpr bool isRegOrMem() const noexcept {
    return Support::isBetween<uint32_t>(opType(), kOpReg, kOpMem);
  }

  //! \}
};

// ============================================================================
// [asmjit::Operand]
// ============================================================================

//! Operand can contain register, memory location, immediate, or label.
class Operand : public Operand_ {
public:
  //! \name Construction & Destruction
  //! \{

  //! Creates `kOpNone` operand having all members initialized to zero.
  constexpr Operand() noexcept
    : Operand_{ kOpNone, 0u, {{ 0u, 0u }}} {}

  //! Creates a cloned `other` operand.
  constexpr Operand(const Operand& other) noexcept = default;

  //! Creates a cloned `other` operand.
  constexpr explicit Operand(const Operand_& other)
    : Operand_(other) {}

  //! Creates an operand initialized to raw `[u0, u1, u2, u3]` values.
  constexpr Operand(Globals::Init_, uint32_t u0, uint32_t u1, uint32_t u2, uint32_t u3) noexcept
    : Operand_{ u0, u1, {{ u2, u3 }}} {}

  //! Creates an uninitialized operand (dangerous).
  inline explicit Operand(Globals::NoInit_) noexcept {}

  //! \}

  //! \name Operator Overloads
  //! \{

  inline Operand& operator=(const Operand& other) noexcept = default;
  inline Operand& operator=(const Operand_& other) noexcept { return operator=(static_cast<const Operand&>(other)); }

  //! \}

  //! \name Utilities
  //! \{

  //! Clones this operand and returns its copy.
  constexpr Operand clone() const noexcept { return Operand(*this); }

  //! \}
};

static_assert(sizeof(Operand) == 16, "asmjit::Operand must be exactly 16 bytes long");

namespace Globals {
  //! A default-constructed operand of `Operand_::kOpNone` type.
  static constexpr const Operand none;
}

// ============================================================================
// [asmjit::Label]
// ============================================================================

//! Label (jump target or data location).
//!
//! Label represents a location in code typically used as a jump target, but
//! may be also a reference to some data or a static variable. Label has to be
//! explicitly created by BaseEmitter.
//!
//! Example of using labels:
//!
//! ```
//! // Create some emitter (for example x86::Assembler).
//! x86::Assembler a;
//!
//! // Create Label instance.
//! Label L1 = a.newLabel();
//!
//! // ... your code ...
//!
//! // Using label.
//! a.jump(L1);
//!
//! // ... your code ...
//!
//! // Bind label to the current position, see `BaseEmitter::bind()`.
//! a.bind(L1);
//! ```
class Label : public Operand {
public:
  //! Type of the Label.
  enum LabelType : uint32_t {
    //! Anonymous (unnamed) label.
    kTypeAnonymous = 0,
    //! Local label (always has parentId).
    kTypeLocal = 1,
    //! Global label (never has parentId).
    kTypeGlobal = 2,
    //! Number of label types.
    kTypeCount = 3
  };

  // TODO: Find a better place, find a better name.
  enum {
    //! Label tag is used as a sub-type, forming a unique signature across all
    //! operand types as 0x1 is never associated with any register (reg-type).
    //! This means that a memory operand's BASE register can be constructed
    //! from virtually any operand (register vs. label) by just assigning its
    //! type (reg type or label-tag) and operand id.
    kLabelTag = 0x1
  };

  //! \name Construction & Destruction
  //! \{

  //! Creates a label operand without ID (you must set the ID to make it valid).
  constexpr Label() noexcept
    : Operand(Globals::Init, kOpLabel, Globals::kInvalidId, 0, 0) {}

  //! Creates a cloned label operand of `other` .
  constexpr Label(const Label& other) noexcept
    : Operand(other) {}

  //! Creates a label operand of the given `id`.
  constexpr explicit Label(uint32_t id) noexcept
    : Operand(Globals::Init, kOpLabel, id, 0, 0) {}

  inline explicit Label(Globals::NoInit_) noexcept
    : Operand(Globals::NoInit) {}

  //! Resets the label, will reset all properties and set its ID to `Globals::kInvalidId`.
  inline void reset() noexcept {
    _signature = kOpLabel;
    _baseId = Globals::kInvalidId;
    _data64 = 0;
  }

  //! \}

  //! \name Overloaded Operators
  //! \{

  inline Label& operator=(const Label& other) noexcept = default;

  //! \}

  //! \name Accessors
  //! \{

  //! Tests whether the label was created by CodeHolder and/or an attached emitter.
  constexpr bool isValid() const noexcept { return _baseId != Globals::kInvalidId; }
  //! Sets the label `id`.
  inline void setId(uint32_t id) noexcept { _baseId = id; }

  //! \}
};

// ============================================================================
// [asmjit::BaseRegTraits]
// ============================================================================

//! \cond INTERNAL
//! Default register traits.
struct BaseRegTraits {
  //! RegType is not valid by default.
  static constexpr uint32_t kValid = 0;
  //! Count of registers (0 if none).
  static constexpr uint32_t kCount = 0;
  //! Everything is void by default.
  static constexpr uint32_t kTypeId = 0;

  //! Zero type by default.
  static constexpr uint32_t kType = 0;
  //! Zero group by default.
  static constexpr uint32_t kGroup = 0;
  //! No size by default.
  static constexpr uint32_t kSize = 0;

  //! Empty signature by default.
  static constexpr uint32_t kSignature = Operand::kOpReg;
};
//! \endcond

// ============================================================================
// [asmjit::BaseReg]
// ============================================================================

//! Structure that allows to extract a register information based on the signature.
//!
//! This information is compatible with operand's signature (32-bit integer)
//! and `RegInfo` just provides easy way to access it.
struct RegInfo {
  inline void reset() noexcept { _signature = 0; }
  inline void setSignature(uint32_t signature) noexcept { _signature = signature; }

  template<uint32_t mask>
  constexpr uint32_t _getSignaturePart() const noexcept {
    return (_signature >> Support::constCtz(mask)) & (mask >> Support::constCtz(mask));
  }

  constexpr bool isValid() const noexcept { return _signature != 0; }
  constexpr uint32_t signature() const noexcept { return _signature; }
  constexpr uint32_t opType() const noexcept { return _getSignaturePart<Operand::kSignatureOpMask>(); }
  constexpr uint32_t group() const noexcept { return _getSignaturePart<Operand::kSignatureRegGroupMask>(); }
  constexpr uint32_t type() const noexcept { return _getSignaturePart<Operand::kSignatureRegTypeMask>(); }
  constexpr uint32_t size() const noexcept { return _getSignaturePart<Operand::kSignatureSizeMask>(); }

  uint32_t _signature;
};

//! Physical/Virtual register operand.
class BaseReg : public Operand {
public:
  //! Architecture neutral register types.
  //!
  //! These must be reused by any platform that contains that types. All GP
  //! and VEC registers are also allowed by design to be part of a BASE|INDEX
  //! of a memory operand.
  enum RegType : uint32_t {
    //! No register - unused, invalid, multiple meanings.
    kTypeNone       = 0,

    // (1 is used as a LabelTag)

    //! 8-bit low general purpose register (X86).
    kTypeGp8Lo = 2,
    //! 8-bit high general purpose register (X86).
    kTypeGp8Hi = 3,
    //! 16-bit general purpose register (X86).
    kTypeGp16 = 4,
    //! 32-bit general purpose register (X86|ARM).
    kTypeGp32 = 5,
    //! 64-bit general purpose register (X86|ARM).
    kTypeGp64 = 6,
    //! 32-bit view of a vector register (ARM).
    kTypeVec32 = 7,
    //! 64-bit view of a vector register (ARM).
    kTypeVec64 = 8,
    //! 128-bit view of a vector register (X86|ARM).
    kTypeVec128 = 9,
    //! 256-bit view of a vector register (X86).
    kTypeVec256 = 10,
    //! 512-bit view of a vector register (X86).
    kTypeVec512 = 11,
    //! 1024-bit view of a vector register (future).
    kTypeVec1024 = 12,
    //! Other0 register, should match `kOther0` group.
    kTypeOther0 = 13,
    //! Other1 register, should match `kOther1` group.
    kTypeOther1 = 14,
    //! Universal id of IP/PC register (if separate).
    kTypeIP = 15,
    //! Start of platform dependent register types (must be honored).
    kTypeCustom = 16,
    //! Maximum possible register id of all architectures.
    kTypeMax = 31
  };

  //! Register group (architecture neutral), and some limits.
  enum RegGroup : uint32_t {
    //! General purpose register group compatible with all backends.
    kGroupGp = 0,
    //! Vector register group compatible with all backends.
    kGroupVec = 1,
    //! Group that is architecture dependent.
    kGroupOther0 = 2,
    //! Group that is architecture dependent.
    kGroupOther1 = 3,
    //! Count of register groups used by virtual registers.
    kGroupVirt = 4,
    //! Count of register groups used by physical registers.
    kGroupCount = 16
  };

  enum Id : uint32_t {
    //! None or any register (mostly internal).
    kIdBad = 0xFFu
  };

  static constexpr uint32_t kSignature = kOpReg;

  //! \name Construction & Destruction
  //! \{

  //! Creates a dummy register operand.
  constexpr BaseReg() noexcept
    : Operand(Globals::Init, kSignature, kIdBad, 0, 0) {}

  //! Creates a new register operand which is the same as `other` .
  constexpr BaseReg(const BaseReg& other) noexcept
    : Operand(other) {}

  //! Creates a new register operand compatible with `other`, but with a different `rId`.
  constexpr BaseReg(const BaseReg& other, uint32_t rId) noexcept
    : Operand(Globals::Init, other._signature, rId, 0, 0) {}

  //! Creates a register initialized to `signature` and `rId`.
  constexpr BaseReg(uint32_t signature, uint32_t rId) noexcept
    : Operand(Globals::Init, signature, rId, 0, 0) {}

  inline explicit BaseReg(Globals::NoInit_) noexcept
    : Operand(Globals::NoInit) {}

  //! \}

  //! \name Overloaded Operators
  //! \{

  inline BaseReg& operator=(const BaseReg& other) noexcept = default;

  //! \}

  //! \name Accessors
  //! \{

  //! Tests whether this register is the same as `other`.
  //!
  //! This is just an optimization. Registers by default only use the first
  //! 8 bytes of the Operand, so this method takes advantage of this knowledge
  //! and only compares these 8 bytes. If both operands were created correctly
  //! then `isEqual()` and `isSame()` should give the same answer, however, if
  //! some one of the two operand contains a garbage or other metadata in the
  //! upper 8 bytes then `isSame()` may return `true` in cases where `isEqual()`
  //! returns false.
  constexpr bool isSame(const BaseReg& other) const noexcept {
    return (_signature == other._signature) &
           (_baseId    == other._baseId   ) ;
  }

  //! Tests whether the register is valid (either virtual or physical).
  constexpr bool isValid() const noexcept { return (_signature != 0) & (_baseId != kIdBad); }

  //! Tests whether this is a physical register.
  constexpr bool isPhysReg() const noexcept { return _baseId < kIdBad; }
  //! Tests whether this is a virtual register.
  constexpr bool isVirtReg() const noexcept { return _baseId > kIdBad; }

  //! Tests whether the register type matches `type` - same as `isReg(type)`, provided for convenience.
  constexpr bool isType(uint32_t type) const noexcept { return (_signature & kSignatureRegTypeMask) == (type << kSignatureRegTypeShift); }
  //! Tests whether the register group matches `group`.
  constexpr bool isGroup(uint32_t group) const noexcept { return (_signature & kSignatureRegGroupMask) == (group << kSignatureRegGroupShift); }

  //! Tests whether the register is a general purpose register (any size).
  constexpr bool isGp() const noexcept { return isGroup(kGroupGp); }
  //! Tests whether the register is a vector register.
  constexpr bool isVec() const noexcept { return isGroup(kGroupVec); }

  using Operand_::isReg;

  //! Same as `isType()`, provided for convenience.
  constexpr bool isReg(uint32_t rType) const noexcept { return isType(rType); }
  //! Tests whether the register type matches `type` and register id matches `rId`.
  constexpr bool isReg(uint32_t rType, uint32_t rId) const noexcept { return isType(rType) && id() == rId; }

  //! Returns the type of the register.
  constexpr uint32_t type() const noexcept { return _getSignaturePart<kSignatureRegTypeMask>(); }
  //! Returns the register group.
  constexpr uint32_t group() const noexcept { return _getSignaturePart<kSignatureRegGroupMask>(); }

  //! Clones the register operand.
  constexpr BaseReg clone() const noexcept { return BaseReg(*this); }

  //! Casts this register to `RegT` by also changing its signature.
  //!
  //! \note Improper use of `cloneAs()` can lead to hard-to-debug errors.
  template<typename RegT>
  constexpr RegT cloneAs() const noexcept { return RegT(RegT::kSignature, id()); }

  //! Casts this register to `other` by also changing its signature.
  //!
  //! \note Improper use of `cloneAs()` can lead to hard-to-debug errors.
  template<typename RegT>
  constexpr RegT cloneAs(const RegT& other) const noexcept { return RegT(other.signature(), id()); }

  //! Sets the register id to `rId`.
  inline void setId(uint32_t rId) noexcept { _baseId = rId; }

  //! Sets a 32-bit operand signature based on traits of `RegT`.
  template<typename RegT>
  inline void setSignatureT() noexcept { _signature = RegT::kSignature; }

  //! Sets the register `signature` and `rId`.
  inline void setSignatureAndId(uint32_t signature, uint32_t rId) noexcept {
    _signature = signature;
    _baseId = rId;
  }

  //! \}

  //! \name Static Functions
  //! \{

  static inline bool isGp(const Operand_& op) noexcept {
    // Check operand type and register group. Not interested in register type and size.
    const uint32_t kSgn = (kOpReg   << kSignatureOpShift      ) |
                          (kGroupGp << kSignatureRegGroupShift) ;
    return (op.signature() & (kSignatureOpMask | kSignatureRegGroupMask)) == kSgn;
  }

  //! Tests whether the `op` operand is either a low or high 8-bit GPB register.
  static inline bool isVec(const Operand_& op) noexcept {
    // Check operand type and register group. Not interested in register type and size.
    const uint32_t kSgn = (kOpReg    << kSignatureOpShift      ) |
                          (kGroupVec << kSignatureRegGroupShift) ;
    return (op.signature() & (kSignatureOpMask | kSignatureRegGroupMask)) == kSgn;
  }

  static inline bool isGp(const Operand_& op, uint32_t rId) noexcept { return isGp(op) & (op.id() == rId); }
  static inline bool isVec(const Operand_& op, uint32_t rId) noexcept { return isVec(op) & (op.id() == rId); }

  //! \}
};

// ============================================================================
// [asmjit::RegOnly]
// ============================================================================

//! RegOnly is 8-byte version of `BaseReg` that allows to store either register
//! or nothing.
//!
//! This class was designed to decrease the space consumed by each extra "operand"
//! in `BaseEmitter` and `InstNode` classes.
struct RegOnly {
  //! Type of the operand, either `kOpNone` or `kOpReg`.
  uint32_t _signature;
  //! Physical or virtual register id.
  uint32_t _id;

  //! \name Construction & Destruction
  //! \{

  //! Initializes the `RegOnly` instance to hold register `signature` and `id`.
  inline void init(uint32_t signature, uint32_t id) noexcept {
    _signature = signature;
    _id = id;
  }

  inline void init(const BaseReg& reg) noexcept { init(reg.signature(), reg.id()); }
  inline void init(const RegOnly& reg) noexcept { init(reg.signature(), reg.id()); }

  //! Resets the `RegOnly` members to zeros (none).
  inline void reset() noexcept { init(0, 0); }

  //! \}

  //! \name Accessors
  //! \{

  //! Tests whether this ExtraReg is none (same as calling `Operand_::isNone()`).
  constexpr bool isNone() const noexcept { return _signature == 0; }
  //! Tests whether the register is valid (either virtual or physical).
  constexpr bool isReg() const noexcept { return _signature != 0; }

  //! Tests whether this is a physical register.
  constexpr bool isPhysReg() const noexcept { return _id < BaseReg::kIdBad; }
  //! Tests whether this is a virtual register (used by `BaseCompiler`).
  constexpr bool isVirtReg() const noexcept { return _id > BaseReg::kIdBad; }

  //! Returns the register signature or 0 if no register is assigned.
  constexpr uint32_t signature() const noexcept { return _signature; }
  //! Returns the register id.
  //!
  //! \note Always check whether the register is assigned before using the
  //! returned identifier as non-assigned `RegOnly` instance would return
  //! zero id, which is still a valid register id.
  constexpr uint32_t id() const noexcept { return _id; }

  //! Sets the register id.
  inline void setId(uint32_t id) noexcept { _id = id; }

  //! \cond INTERNAL
  //!
  //! Extracts information from operand's signature.
  template<uint32_t mask>
  constexpr uint32_t _getSignaturePart() const noexcept {
    return (_signature >> Support::constCtz(mask)) & (mask >> Support::constCtz(mask));
  }
  //! \endcond

  //! Returns the type of the register.
  constexpr uint32_t type() const noexcept { return _getSignaturePart<Operand::kSignatureRegTypeMask>(); }
  //! Returns the register group.
  constexpr uint32_t group() const noexcept { return _getSignaturePart<Operand::kSignatureRegGroupMask>(); }

  //! \}

  //! \name Utilities
  //! \{

  //! Converts this ExtraReg to a real `RegT` operand.
  template<typename RegT>
  constexpr RegT toReg() const noexcept { return RegT(_signature, _id); }

  //! \}
};

// ============================================================================
// [asmjit::BaseMem]
// ============================================================================

//! Base class for all memory operands.
//!
//! \note It's tricky to pack all possible cases that define a memory operand
//! into just 16 bytes. The `BaseMem` splits data into the following parts:
//!
//!   BASE   - Base register or label - requires 36 bits total. 4 bits are used to
//!            encode the type of the BASE operand (label vs. register type) and
//!            the remaining 32 bits define the BASE id, which can be a physical or
//!            virtual register index. If BASE type is zero, which is never used as
//!            a register-type and label doesn't use it as well then BASE field
//!            contains a high DWORD of a possible 64-bit absolute address, which is
//!            possible on X64.
//!
//!   INDEX  - Index register (or theoretically Label, which doesn't make sense).
//!            Encoding is similar to BASE - it also requires 36 bits and splits
//!            the encoding to INDEX type (4 bits defining the register type) and
//!            id (32-bits).
//!
//!   OFFSET - A relative offset of the address. Basically if BASE is specified
//!            the relative displacement adjusts BASE and an optional INDEX. if
//!            BASE is not specified then the OFFSET should be considered as ABSOLUTE
//!            address (at least on X86). In that case its low 32 bits are stored in
//!            DISPLACEMENT field and the remaining high 32 bits are stored in BASE.
//!
//!   OTHER  - There is rest 8 bits that can be used for whatever purpose. The
//!            x86::Mem operand uses these bits to store segment override prefix and
//!            index shift (scale).
class BaseMem : public Operand {
public:
  enum AddrType : uint32_t {
    kAddrTypeDefault = 0,
    kAddrTypeAbs     = 1,
    kAddrTypeRel     = 2
  };

  // Shortcuts.
  enum SignatureMem : uint32_t {
    kSignatureMemAbs = kAddrTypeAbs << kSignatureMemAddrTypeShift,
    kSignatureMemRel = kAddrTypeRel << kSignatureMemAddrTypeShift
  };

  //! \cond INTERNAL
  //! Used internally to construct `BaseMem` operand from decomposed data.
  struct Decomposed {
    uint32_t baseType;
    uint32_t baseId;
    uint32_t indexType;
    uint32_t indexId;
    int32_t offset;
    uint32_t size;
    uint32_t flags;
  };
  //! \endcond

  //! \name Construction & Destruction
  //! \{

  //! Creates a default `BaseMem` operand, that points to [0].
  constexpr BaseMem() noexcept
    : Operand(Globals::Init, kOpMem, 0, 0, 0) {}

  //! Creates a `BaseMem` operand that is a clone of `other`.
  constexpr BaseMem(const BaseMem& other) noexcept
    : Operand(other) {}

  //! \cond INTERNAL

  //! Creates a `BaseMem` operand from 4 integers as used by `Operand_` struct.
  constexpr BaseMem(Globals::Init_, uint32_t u0, uint32_t u1, uint32_t u2, uint32_t u3) noexcept
    : Operand(Globals::Init, u0, u1, u2, u3) {}

  constexpr BaseMem(const Decomposed& d) noexcept
    : Operand(Globals::Init,
              kOpMem | (d.baseType  << kSignatureMemBaseTypeShift )
                     | (d.indexType << kSignatureMemIndexTypeShift)
                     | (d.size      << kSignatureSizeShift        )
                     | d.flags,
              d.baseId,
              d.indexId,
              uint32_t(d.offset)) {}

  //! \endcond

  //! Creates a completely uninitialized `BaseMem` operand.
  inline explicit BaseMem(Globals::NoInit_) noexcept
    : Operand(Globals::NoInit) {}

  //! Resets the memory operand - after the reset the memory points to [0].
  inline void reset() noexcept {
    _signature = kOpMem;
    _baseId = 0;
    _data64 = 0;
  }

  //! \}

  //! \name Overloaded Operators
  //! \{

  inline BaseMem& operator=(const BaseMem& other) noexcept { copyFrom(other); return *this; }

  //! \}

  //! \name Accessors
  //! \{

  //! Clones the memory operand.
  constexpr BaseMem clone() const noexcept { return BaseMem(*this); }

  constexpr uint32_t addrType() const noexcept { return _getSignaturePart<kSignatureMemAddrTypeMask>(); }
  inline void setAddrType(uint32_t addrType) noexcept { _setSignaturePart<kSignatureMemAddrTypeMask>(addrType); }
  inline void resetAddrType() noexcept { _setSignaturePart<kSignatureMemAddrTypeMask>(0); }

  constexpr bool isAbs() const noexcept { return addrType() == kAddrTypeAbs; }
  inline void setAbs() noexcept { setAddrType(kAddrTypeAbs); }

  constexpr bool isRel() const noexcept { return addrType() == kAddrTypeRel; }
  inline void setRel() noexcept { setAddrType(kAddrTypeRel); }

  constexpr bool isRegHome() const noexcept { return _hasSignaturePart<kSignatureMemRegHomeFlag>(); }
  inline void setRegHome() noexcept { _signature |= kSignatureMemRegHomeFlag; }
  inline void clearRegHome() noexcept { _signature &= ~kSignatureMemRegHomeFlag; }

  //! Tests whether the memory operand has a BASE register or label specified.
  constexpr bool hasBase() const noexcept { return (_signature & kSignatureMemBaseTypeMask) != 0; }
  //! Tests whether the memory operand has an INDEX register specified.
  constexpr bool hasIndex() const noexcept { return (_signature & kSignatureMemIndexTypeMask) != 0; }
  //! Tests whether the memory operand has BASE and INDEX register.
  constexpr bool hasBaseOrIndex() const noexcept { return (_signature & kSignatureMemBaseIndexMask) != 0; }
  //! Tests whether the memory operand has BASE and INDEX register.
  constexpr bool hasBaseAndIndex() const noexcept { return (_signature & kSignatureMemBaseTypeMask) != 0 && (_signature & kSignatureMemIndexTypeMask) != 0; }

  //! Tests whether the BASE operand is a register (registers start after `kLabelTag`).
  constexpr bool hasBaseReg() const noexcept { return (_signature & kSignatureMemBaseTypeMask) > (Label::kLabelTag << kSignatureMemBaseTypeShift); }
  //! Tests whether the BASE operand is a label.
  constexpr bool hasBaseLabel() const noexcept { return (_signature & kSignatureMemBaseTypeMask) == (Label::kLabelTag << kSignatureMemBaseTypeShift); }
  //! Tests whether the INDEX operand is a register (registers start after `kLabelTag`).
  constexpr bool hasIndexReg() const noexcept { return (_signature & kSignatureMemIndexTypeMask) > (Label::kLabelTag << kSignatureMemIndexTypeShift); }

  //! Returns the type of the BASE register (0 if this memory operand doesn't
  //! use the BASE register).
  //!
  //! \note If the returned type is one (a value never associated to a register
  //! type) the BASE is not register, but it's a label. One equals to `kLabelTag`.
  //! You should always check `hasBaseLabel()` before using `baseId()` result.
  constexpr uint32_t baseType() const noexcept { return _getSignaturePart<kSignatureMemBaseTypeMask>(); }

  //! Returns the type of an INDEX register (0 if this memory operand doesn't
  //! use the INDEX register).
  constexpr uint32_t indexType() const noexcept { return _getSignaturePart<kSignatureMemIndexTypeMask>(); }

  //! This is used internally for BASE+INDEX validation.
  constexpr uint32_t baseAndIndexTypes() const noexcept { return _getSignaturePart<kSignatureMemBaseIndexMask>(); }

  //! Returns both BASE (4:0 bits) and INDEX (9:5 bits) types combined into a
  //! single value.
  //!
  //! \remarks Returns id of the BASE register or label (if the BASE was
  //! specified as label).
  constexpr uint32_t baseId() const noexcept { return _baseId; }

  //! Returns the id of the INDEX register.
  constexpr uint32_t indexId() const noexcept { return _mem.indexId; }

  //! Sets the id of the BASE register (without modifying its type).
  inline void setBaseId(uint32_t rId) noexcept { _baseId = rId; }
  //! Sets the id of the INDEX register (without modifying its type).
  inline void setIndexId(uint32_t rId) noexcept { _mem.indexId = rId; }

  //! Sets the base register to type and id of the given `base` operand.
  inline void setBase(const BaseReg& base) noexcept { return _setBase(base.type(), base.id()); }
  //! Sets the index register to type and id of the given `index` operand.
  inline void setIndex(const BaseReg& index) noexcept { return _setIndex(index.type(), index.id()); }

  inline void _setBase(uint32_t rType, uint32_t rId) noexcept {
    _setSignaturePart<kSignatureMemBaseTypeMask>(rType);
    _baseId = rId;
  }

  inline void _setIndex(uint32_t rType, uint32_t rId) noexcept {
    _setSignaturePart<kSignatureMemIndexTypeMask>(rType);
    _mem.indexId = rId;
  }

  //! Resets the memory operand's BASE register or label.
  inline void resetBase() noexcept { _setBase(0, 0); }
  //! Resets the memory operand's INDEX register.
  inline void resetIndex() noexcept { _setIndex(0, 0); }

  //! Sets the memory operand size (in bytes).
  inline void setSize(uint32_t size) noexcept { _setSignaturePart<kSignatureSizeMask>(size); }

  //! Tests whether the memory operand has a 64-bit offset or absolute address.
  //!
  //! If this is true then `hasBase()` must always report false.
  constexpr bool isOffset64Bit() const noexcept { return baseType() == 0; }

  //! Tests whether the memory operand has a non-zero offset or absolute address.
  constexpr bool hasOffset() const noexcept {
    return (_mem.offsetLo32 | uint32_t(_baseId & Support::bitMaskFromBool<uint32_t>(isOffset64Bit()))) != 0;
  }

  //! Returns either relative offset or absolute address as 64-bit integer.
  constexpr int64_t offset() const noexcept {
    return isOffset64Bit() ? int64_t(uint64_t(_mem.offsetLo32) | (uint64_t(_baseId) << 32))
                           : int64_t(int32_t(_mem.offsetLo32)); // Sign extend 32-bit offset.
  }

  //! Returns a 32-bit low part of a 64-bit offset or absolute address.
  constexpr int32_t offsetLo32() const noexcept { return int32_t(_mem.offsetLo32); }
  //! Returns a 32-but high part of a 64-bit offset or absolute address.
  //!
  //! \note This function is UNSAFE and returns garbage if `isOffset64Bit()`
  //! returns false. Never use it blindly without checking it first.
  constexpr int32_t offsetHi32() const noexcept { return int32_t(_baseId); }

  //! Sets a 64-bit offset or an absolute address to `offset`.
  //!
  //! \note This functions attempts to set both high and low parts of a 64-bit
  //! offset, however, if the operand has a BASE register it will store only the
  //! low 32 bits of the offset / address as there is no way to store both BASE
  //! and 64-bit offset, and there is currently no architecture that has such
  //! capability targeted by AsmJit.
  inline void setOffset(int64_t offset) noexcept {
    uint32_t lo = uint32_t(uint64_t(offset) & 0xFFFFFFFFu);
    uint32_t hi = uint32_t(uint64_t(offset) >> 32);
    uint32_t hiMsk = Support::bitMaskFromBool<uint32_t>(isOffset64Bit());

    _mem.offsetLo32 = lo;
    _baseId = (hi & hiMsk) | (_baseId & ~hiMsk);
  }
  //! Sets a low 32-bit offset to `offset` (don't use without knowing how BaseMem works).
  inline void setOffsetLo32(int32_t offset) noexcept { _mem.offsetLo32 = uint32_t(offset); }

  //! Adjusts the offset by `offset`.
  //!
  //! \note This is a fast function that doesn't use the HI 32-bits of a
  //! 64-bit offset. Use it only if you know that there is a BASE register
  //! and the offset is only 32 bits anyway.

  //! Adjusts the offset by a 64-bit `offset`.
  inline void addOffset(int64_t offset) noexcept {
    if (isOffset64Bit()) {
      int64_t result = offset + int64_t(uint64_t(_mem.offsetLo32) | (uint64_t(_baseId) << 32));
      _mem.offsetLo32 = uint32_t(uint64_t(result) & 0xFFFFFFFFu);
      _baseId         = uint32_t(uint64_t(result) >> 32);
    }
    else {
      _mem.offsetLo32 += uint32_t(uint64_t(offset) & 0xFFFFFFFFu);
    }
  }

  //! Adds `offset` to a low 32-bit offset part (don't use without knowing how
  //! BaseMem works).
  inline void addOffsetLo32(int32_t offset) noexcept { _mem.offsetLo32 += uint32_t(offset); }

  //! Resets the memory offset to zero.
  inline void resetOffset() noexcept { setOffset(0); }

  //! Resets the lo part of the memory offset to zero (don't use without knowing
  //! how BaseMem works).
  inline void resetOffsetLo32() noexcept { setOffsetLo32(0); }

  //! \}
};

// ============================================================================
// [asmjit::Imm]
// ============================================================================

//! Immediate operand.
//!
//! Immediate operand is usually part of instruction itself. It's inlined after
//! or before the instruction opcode. Immediates can be only signed or unsigned
//! integers.
//!
//! To create an immediate operand use `asmjit::imm()` helper, which can be used
//! with any type, not just the default 64-bit int.
class Imm : public Operand {
public:
  //! \name Construction & Destruction
  //! \{

  //! Creates a new immediate value (initial value is 0).
  constexpr Imm() noexcept
    : Operand(Globals::Init, kOpImm, 0, 0, 0) {}

  //! Creates a new immediate value from `other`.
  constexpr Imm(const Imm& other) noexcept
    : Operand(other) {}

  //! Creates a new signed immediate value, assigning the value to `val`.
  constexpr explicit Imm(int64_t val) noexcept
    : Operand(Globals::Init, kOpImm, 0, Support::unpackU32At0(val), Support::unpackU32At1(val)) {}

  inline explicit Imm(Globals::NoInit_) noexcept
    : Operand(Globals::NoInit) {}

  //! \}

  //! \name Overloaded Operators
  //! \{

  //! Assigns the value of the `other` operand to this immediate.
  inline Imm& operator=(const Imm& other) noexcept { copyFrom(other); return *this; }

  //! \}

  //! \name Accessors
  //! \{

  //! Tests whether the immediate can be casted to 8-bit signed integer.
  constexpr bool isInt8() const noexcept { return Support::isInt8(int64_t(_data64)); }
  //! Tests whether the immediate can be casted to 8-bit unsigned integer.
  constexpr bool isUInt8() const noexcept { return Support::isUInt8(int64_t(_data64)); }
  //! Tests whether the immediate can be casted to 16-bit signed integer.
  constexpr bool isInt16() const noexcept { return Support::isInt16(int64_t(_data64)); }
  //! Tests whether the immediate can be casted to 16-bit unsigned integer.
  constexpr bool isUInt16() const noexcept { return Support::isUInt16(int64_t(_data64)); }
  //! Tests whether the immediate can be casted to 32-bit signed integer.
  constexpr bool isInt32() const noexcept { return Support::isInt32(int64_t(_data64)); }
  //! Tests whether the immediate can be casted to 32-bit unsigned integer.
  constexpr bool isUInt32() const noexcept { return Support::isUInt32(int64_t(_data64)); }

  //! Returns immediate value as 8-bit signed integer, possibly cropped.
  constexpr int8_t i8() const noexcept { return int8_t(_data64 & 0xFFu); }
  //! Returns immediate value as 8-bit unsigned integer, possibly cropped.
  constexpr uint8_t u8() const noexcept { return uint8_t(_data64 & 0xFFu); }
  //! Returns immediate value as 16-bit signed integer, possibly cropped.
  constexpr int16_t i16() const noexcept { return int16_t(_data64 & 0xFFFFu);}
  //! Returns immediate value as 16-bit unsigned integer, possibly cropped.
  constexpr uint16_t u16() const noexcept { return uint16_t(_data64 & 0xFFFFu);}
  //! Returns immediate value as 32-bit signed integer, possibly cropped.
  constexpr int32_t i32() const noexcept { return int32_t(_data64 & 0xFFFFFFFFu); }
  //! Returns low 32-bit signed integer.
  constexpr int32_t i32Lo() const noexcept { return int32_t(_data64 & 0xFFFFFFFFu); }
  //! Returns high 32-bit signed integer.
  constexpr int32_t i32Hi() const noexcept { return int32_t(_data64 >> 32); }
  //! Returns immediate value as 32-bit unsigned integer, possibly cropped.
  constexpr uint32_t u32() const noexcept { return uint32_t(_data64 & 0xFFFFFFFFu); }
  //! Returns low 32-bit signed integer.
  constexpr uint32_t u32Lo() const noexcept { return uint32_t(_data64 & 0xFFFFFFFFu); }
  //! Returns high 32-bit signed integer.
  constexpr uint32_t u32Hi() const noexcept { return uint32_t(_data64 >> 32); }
  //! Returns immediate value as 64-bit signed integer.
  constexpr int64_t i64() const noexcept { return int64_t(_data64); }
  //! Returns immediate value as 64-bit unsigned integer.
  constexpr uint64_t u64() const noexcept { return _data64; }
  //! Returns immediate value as `intptr_t`, possibly cropped if size of `intptr_t` is 32 bits.
  constexpr intptr_t iptr() const noexcept { return (sizeof(intptr_t) == sizeof(int64_t)) ? intptr_t(_data64) : intptr_t(i32()); }
  //! Returns immediate value as `uintptr_t`, possibly cropped if size of `uintptr_t` is 32 bits.
  constexpr uintptr_t uptr() const noexcept { return (sizeof(uintptr_t) == sizeof(uint64_t)) ? uintptr_t(_data64) : uintptr_t(u32()); }

  //! Sets immediate value to 8-bit signed integer `val`.
  inline void setI8(int8_t val) noexcept { _data64 = uint64_t(int64_t(val)); }
  //! Sets immediate value to 8-bit unsigned integer `val`.
  inline void setU8(uint8_t val) noexcept { _data64 = uint64_t(val); }
  //! Sets immediate value to 16-bit signed integer `val`.
  inline void setI16(int16_t val) noexcept { _data64 = uint64_t(int64_t(val)); }
  //! Sets immediate value to 16-bit unsigned integer `val`.
  inline void setU16(uint16_t val) noexcept { _data64 = uint64_t(val); }
  //! Sets immediate value to 32-bit signed integer `val`.
  inline void setI32(int32_t val) noexcept { _data64 = uint64_t(int64_t(val)); }
  //! Sets immediate value to 32-bit unsigned integer `val`.
  inline void setU32(uint32_t val) noexcept { _data64 = uint64_t(val); }
  //! Sets immediate value to 64-bit signed integer `val`.
  inline void setI64(int64_t val) noexcept { _data64 = uint64_t(val); }
  //! Sets immediate value to 64-bit unsigned integer `val`.
  inline void setU64(uint64_t val) noexcept { _data64 = val; }
  //! Sets immediate value to intptr_t `val`.
  inline void setIPtr(intptr_t val) noexcept { _data64 = uint64_t(int64_t(val)); }
  //! Sets immediate value to uintptr_t `val`.
  inline void setUPtr(uintptr_t val) noexcept { _data64 = uint64_t(val); }

  //! Sets immediate value to `val`.
  template<typename T>
  inline void setValue(T val) noexcept { setI64(int64_t(Support::asNormalized(val))); }

  inline void setDouble(double d) noexcept {
    _data64 = Support::bitCast<uint64_t>(d);
  }

  //! \}

  //! \name Utilities
  //! \{

  //! Clones the immediate operand.
  constexpr Imm clone() const noexcept { return Imm(*this); }

  inline void signExtend8Bits() noexcept { _data64 = uint64_t(int64_t(i8())); }
  inline void signExtend16Bits() noexcept { _data64 = uint64_t(int64_t(i16())); }
  inline void signExtend32Bits() noexcept { _data64 = uint64_t(int64_t(i32())); }

  inline void zeroExtend8Bits() noexcept { _data64 &= 0x000000FFu; }
  inline void zeroExtend16Bits() noexcept { _data64 &= 0x0000FFFFu; }
  inline void zeroExtend32Bits() noexcept { _data64 &= 0xFFFFFFFFu; }

  //! \}
};

//! Creates a new immediate operand.
//!
//! Using `imm(x)` is much nicer than using `Imm(x)` as this is a template
//! which can accept any integer including pointers and function pointers.
template<typename T>
static constexpr Imm imm(T val) noexcept {
  return Imm(std::is_signed<T>::value ? int64_t(val) : int64_t(uint64_t(val)));
}

//! \}

ASMJIT_END_NAMESPACE

#endif // _ASMJIT_CORE_OPERAND_H
