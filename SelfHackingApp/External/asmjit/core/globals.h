// [AsmJit]
// Machine Code Generation for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

#ifndef _ASMJIT_CORE_GLOBALS_H
#define _ASMJIT_CORE_GLOBALS_H

#include "../core/build.h"

ASMJIT_BEGIN_NAMESPACE

// ============================================================================
// [asmjit::Support]
// ============================================================================

//! \cond INTERNAL
//! \addtogroup Support
//! \{
namespace Support {
  //! Cast designed to cast between function and void* pointers.
  template<typename Dst, typename Src>
  static inline Dst ptr_cast_impl(Src p) noexcept { return (Dst)p; }
} // {Support}

#if defined(ASMJIT_NO_STDCXX)
namespace Support {
  ASMJIT_INLINE void* operatorNew(size_t n) noexcept { return malloc(n); }
  ASMJIT_INLINE void operatorDelete(void* p) noexcept { if (p) free(p); }
} // {Support}

#define ASMJIT_BASE_CLASS(TYPE)                                               \
  ASMJIT_INLINE void* operator new(size_t n) noexcept {                       \
    return Support::operatorNew(n);                                           \
  }                                                                           \
                                                                              \
  ASMJIT_INLINE void  operator delete(void* p) noexcept {                     \
    Support::operatorDelete(p);                                               \
  }                                                                           \
                                                                              \
  ASMJIT_INLINE void* operator new(size_t, void* p) noexcept { return p; }    \
  ASMJIT_INLINE void  operator delete(void*, void*) noexcept {}
#else
#define ASMJIT_BASE_CLASS(TYPE)
#endif

//! \}
//! \endcond

// ============================================================================
// [asmjit::Globals]
// ============================================================================

//! \addtogroup asmjit_core
//! \{

//! Contains typedefs, constants, and variables used globally by AsmJit.
namespace Globals {

// ============================================================================
// [asmjit::Globals::<global>]
// ============================================================================

//! Host memory allocator overhead.
constexpr uint32_t kAllocOverhead = uint32_t(sizeof(intptr_t) * 4);

//! Host memory allocator alignment.
constexpr uint32_t kAllocAlignment = 8;

//! Aggressive growing strategy threshold.
constexpr uint32_t kGrowThreshold = 1024 * 1024 * 16;

//! Maximum height of RB-Tree is:
//!
//!   `2 * log2(n + 1)`.
//!
//! Size of RB node is at least two pointers (without data),
//! so a theoretical architecture limit would be:
//!
//!   `2 * log2(addressableMemorySize / sizeof(Node) + 1)`
//!
//! Which yields 30 on 32-bit arch and 61 on 64-bit arch.
//! The final value was adjusted by +1 for safety reasons.
constexpr uint32_t kMaxTreeHeight = (ASMJIT_ARCH_BITS == 32 ? 30 : 61) + 1;

//! Maximum number of operands per a single instruction.
constexpr uint32_t kMaxOpCount = 6;

// TODO: Use this one.
constexpr uint32_t kMaxFuncArgs = 16;

//! Maximum number of physical registers AsmJit can use per register group.
constexpr uint32_t kMaxPhysRegs = 32;

//! Maximum alignment.
constexpr uint32_t kMaxAlignment = 64;

//! Maximum label or symbol size in bytes.
constexpr uint32_t kMaxLabelNameSize = 2048;

//! Maximum section name size.
constexpr uint32_t kMaxSectionNameSize = 35;

//! Maximum size of comment.
constexpr uint32_t kMaxCommentSize = 1024;

//! Invalid identifier.
constexpr uint32_t kInvalidId = 0xFFFFFFFFu;

//! Returned by `indexOf()` and similar when working with containers that use 32-bit index/size.
constexpr uint32_t kNotFound = 0xFFFFFFFFu;

//! Invalid base address.
constexpr uint64_t kNoBaseAddress = ~uint64_t(0);

// ============================================================================
// [asmjit::Globals::ResetPolicy]
// ============================================================================

//! Reset policy used by most `reset()` functions.
enum ResetPolicy : uint32_t {
  //! Soft reset, doesn't deallocate memory (default).
  kResetSoft = 0,
  //! Hard reset, releases all memory used, if any.
  kResetHard = 1
};

// ============================================================================
// [asmjit::Globals::Link]
// ============================================================================

enum Link : uint32_t {
  kLinkLeft  = 0,
  kLinkRight = 1,

  kLinkPrev  = 0,
  kLinkNext  = 1,

  kLinkFirst = 0,
  kLinkLast  = 1,

  kLinkCount = 2
};

struct Init_ {};
struct NoInit_ {};

static const constexpr Init_ Init {};
static const constexpr NoInit_ NoInit {};

} // {Globals}

// ============================================================================
// [asmjit::Error]
// ============================================================================

//! AsmJit error type (uint32_t).
typedef uint32_t Error;

//! AsmJit error codes.
enum ErrorCode : uint32_t {
  //! No error (success).
  kErrorOk = 0,

  //! Out of memory.
  kErrorOutOfMemory,

  //! Invalid argument.
  kErrorInvalidArgument,

  //! Invalid state.
  //!
  //! If this error is returned it means that either you are doing something
  //! wrong or AsmJit caught itself by doing something wrong. This error should
  //! never be ignored.
  kErrorInvalidState,

  //! Invalid or incompatible architecture.
  kErrorInvalidArch,

  //! The object is not initialized.
  kErrorNotInitialized,
  //! The object is already initialized.
  kErrorAlreadyInitialized,

  //! Built-in feature was disabled at compile time and it's not available.
  kErrorFeatureNotEnabled,

  //! Too many handles (Windows) or file descriptors (Unix/Posix).
  kErrorTooManyHandles,
  //! Code generated is larger than allowed.
  kErrorTooLarge,

  //! No code generated.
  //!
  //! Returned by runtime if the `CodeHolder` contains no code.
  kErrorNoCodeGenerated,

  //! Invalid directive.
  kErrorInvalidDirective,
  //! Attempt to use uninitialized label.
  kErrorInvalidLabel,
  //! Label index overflow - a single `Assembler` instance can hold almost
  //! 2^32 (4 billion) labels. If there is an attempt to create more labels
  //! then this error is returned.
  kErrorTooManyLabels,
  //! Label is already bound.
  kErrorLabelAlreadyBound,
  //! Label is already defined (named labels).
  kErrorLabelAlreadyDefined,
  //! Label name is too long.
  kErrorLabelNameTooLong,
  //! Label must always be local if it's anonymous (without a name).
  kErrorInvalidLabelName,
  //! Parent id passed to `CodeHolder::newNamedLabelId()` was invalid.
  kErrorInvalidParentLabel,
  //! Parent id specified for a non-local (global) label.
  kErrorNonLocalLabelCantHaveParent,

  //! Invalid section.
  kErrorInvalidSection,
  //! Too many sections (section index overflow).
  kErrorTooManySections,
  //! Invalid section name (most probably too long).
  kErrorInvalidSectionName,

  //! Relocation index overflow (too many relocations).
  kErrorTooManyRelocations,
  //! Invalid relocation entry.
  kErrorInvalidRelocEntry,
  //! Reloc entry contains address that is out of range (unencodable).
  kErrorRelocOffsetOutOfRange,

  //! Invalid assignment to a register, function argument, or function return value.
  kErrorInvalidAssignment,
  //! Invalid instruction.
  kErrorInvalidInstruction,
  //! Invalid register type.
  kErrorInvalidRegType,
  //! Invalid register group.
  kErrorInvalidRegGroup,
  //! Invalid register's physical id.
  kErrorInvalidPhysId,
  //! Invalid register's virtual id.
  kErrorInvalidVirtId,
  //! Invalid prefix combination.
  kErrorInvalidPrefixCombination,
  //! Invalid LOCK prefix.
  kErrorInvalidLockPrefix,
  //! Invalid XACQUIRE prefix.
  kErrorInvalidXAcquirePrefix,
  //! Invalid XRELEASE prefix.
  kErrorInvalidXReleasePrefix,
  //! Invalid REP prefix.
  kErrorInvalidRepPrefix,
  //! Invalid REX prefix.
  kErrorInvalidRexPrefix,
  //! Invalid {...} register.
  kErrorInvalidExtraReg,
  //! Invalid {k} use (not supported by the instruction).
  kErrorInvalidKMaskUse,
  //! Invalid {k}{z} use (not supported by the instruction).
  kErrorInvalidKZeroUse,
  //! Invalid broadcast - Currently only related to invalid use of AVX-512 {1tox}.
  kErrorInvalidBroadcast,
  //! Invalid 'embedded-rounding' {er} or 'suppress-all-exceptions' {sae} (AVX-512).
  kErrorInvalidEROrSAE,
  //! Invalid address used (not encodable).
  kErrorInvalidAddress,
  //! Invalid index register used in memory address (not encodable).
  kErrorInvalidAddressIndex,
  //! Invalid address scale (not encodable).
  kErrorInvalidAddressScale,
  //! Invalid use of 64-bit address.
  kErrorInvalidAddress64Bit,
  //! Invalid use of 64-bit address that require 32-bit zero-extension (X64).
  kErrorInvalidAddress64BitZeroExtension,
  //! Invalid displacement (not encodable).
  kErrorInvalidDisplacement,
  //! Invalid segment (X86).
  kErrorInvalidSegment,

  //! Invalid immediate (out of bounds on X86 and invalid pattern on ARM).
  kErrorInvalidImmediate,

  //! Invalid operand size.
  kErrorInvalidOperandSize,
  //! Ambiguous operand size (memory has zero size while it's required to determine the operation type.
  kErrorAmbiguousOperandSize,
  //! Mismatching operand size (size of multiple operands doesn't match the operation size).
  kErrorOperandSizeMismatch,

  //! Invalid option.
  kErrorInvalidOption,
  //! Option already defined.
  kErrorOptionAlreadyDefined,

  //! Invalid TypeId.
  kErrorInvalidTypeId,
  //! Invalid use of a 8-bit GPB-HIGH register.
  kErrorInvalidUseOfGpbHi,
  //! Invalid use of a 64-bit GPQ register in 32-bit mode.
  kErrorInvalidUseOfGpq,
  //! Invalid use of an 80-bit float (Type::kIdF80).
  kErrorInvalidUseOfF80,
  //! Some registers in the instruction muse be consecutive (some ARM and AVX512 neural-net instructions).
  kErrorNotConsecutiveRegs,

  //! AsmJit requires a physical register, but no one is available.
  kErrorNoMorePhysRegs,
  //! A variable has been assigned more than once to a function argument (BaseCompiler).
  kErrorOverlappedRegs,
  //! Invalid register to hold stack arguments offset.
  kErrorOverlappingStackRegWithRegArg,

  //! Unbound label cannot be evaluated by expression.
  kErrorExpressionLabelNotBound,
  //! Arithmetic overflow during expression evaluation.
  kErrorExpressionOverflow,

  //! Count of AsmJit error codes.
  kErrorCount
};

// ============================================================================
// [asmjit::ByteOrder]
// ============================================================================

//! Byte order.
namespace ByteOrder {
  enum : uint32_t {
    kLE      = 0,
    kBE      = 1,
    kNative  = ASMJIT_ARCH_LE ? kLE : kBE,
    kSwapped = ASMJIT_ARCH_LE ? kBE : kLE
  };
}

// ============================================================================
// [asmjit::ptr_as_func / func_as_ptr]
// ============================================================================

template<typename Func>
static inline Func ptr_as_func(void* func) noexcept { return Support::ptr_cast_impl<Func, void*>(func); }
template<typename Func>
static inline void* func_as_ptr(Func func) noexcept { return Support::ptr_cast_impl<void*, Func>(func); }

// ============================================================================
// [asmjit::DebugUtils]
// ============================================================================

//! Debugging utilities.
namespace DebugUtils {

//! Returns the error `err` passed.
//!
//! Provided for debugging purposes. Putting a breakpoint inside `errored` can
//! help with tracing the origin of any error reported / returned by AsmJit.
static constexpr Error errored(Error err) noexcept { return err; }

//! Returns a printable version of `asmjit::Error` code.
ASMJIT_API const char* errorAsString(Error err) noexcept;

//! Called to output debugging message(s).
ASMJIT_API void debugOutput(const char* str) noexcept;

//! Called on assertion failure.
//!
//! \param file Source file name where it happened.
//! \param line Line in the source file.
//! \param msg Message to display.
//!
//! If you have problems with assertions put a breakpoint at assertionFailed()
//! function (asmjit/core/globals.cpp) and check the call stack to locate the
//! failing code.
ASMJIT_API void ASMJIT_NORETURN assertionFailed(const char* file, int line, const char* msg) noexcept;

#if defined(ASMJIT_BUILD_DEBUG)
#define ASMJIT_ASSERT(EXP)                                                     \
  do {                                                                         \
    if (ASMJIT_LIKELY(EXP))                                                    \
      break;                                                                   \
    ::asmjit::DebugUtils::assertionFailed(__FILE__, __LINE__, #EXP);           \
  } while (0)
#else
#define ASMJIT_ASSERT(EXP) ((void)0)
#endif

//! Used by AsmJit to propagate a possible `Error` produced by `...` to the caller.
#define ASMJIT_PROPAGATE(...)               \
  do {                                      \
    ::asmjit::Error _err = __VA_ARGS__;     \
    if (ASMJIT_UNLIKELY(_err))              \
      return _err;                          \
  } while (0)

} // {DebugUtils}

//! \}

ASMJIT_END_NAMESPACE

#endif // _ASMJIT_CORE_GLOBALS_H
