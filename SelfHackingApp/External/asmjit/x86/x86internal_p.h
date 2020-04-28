// [AsmJit]
// Machine Code Generation for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

#ifndef _ASMJIT_X86_X86INTERNAL_P_H
#define _ASMJIT_X86_X86INTERNAL_P_H

#include "../core/build.h"

#include "../core/func.h"
#include "../x86/x86emitter.h"
#include "../x86/x86operand.h"

ASMJIT_BEGIN_SUB_NAMESPACE(x86)

//! \cond INTERNAL
//! \addtogroup asmjit_x86
//! \{

// ============================================================================
// [asmjit::X86Internal]
// ============================================================================

//! X86 utilities used at multiple places, not part of public API, not exported.
struct X86Internal {
  //! Initialize `FuncDetail` (X86 specific).
  static Error initFuncDetail(FuncDetail& func, const FuncSignature& sign, uint32_t gpSize) noexcept;

  //! Initialize `FuncFrame` (X86 specific).
  static Error initFuncFrame(FuncFrame& frame, const FuncDetail& func) noexcept;

  //! Finalize `FuncFrame` (X86 specific).
  static Error finalizeFuncFrame(FuncFrame& frame) noexcept;

  static Error argsToFuncFrame(const FuncArgsAssignment& args, FuncFrame& frame) noexcept;

  //! Emit function prolog.
  static Error emitProlog(Emitter* emitter, const FuncFrame& frame);

  //! Emit function epilog.
  static Error emitEpilog(Emitter* emitter, const FuncFrame& frame);

  //! Emit a pure move operation between two registers or the same type or
  //! between a register and its home slot. This function does not handle
  //! register conversion.
  static Error emitRegMove(Emitter* emitter,
    const Operand_& dst_,
    const Operand_& src_, uint32_t typeId, bool avxEnabled, const char* comment = nullptr);

  //! Emit move from a function argument (either register or stack) to a register.
  //!
  //! This function can handle the necessary conversion from one argument to
  //! another, and from one register type to another, if it's possible. Any
  //! attempt of conversion that requires third register of a different group
  //! (for example conversion from K to MMX) will fail.
  static Error emitArgMove(Emitter* emitter,
    const Reg& dst_, uint32_t dstTypeId,
    const Operand_& src_, uint32_t srcTypeId, bool avxEnabled, const char* comment = nullptr);

  static Error emitArgsAssignment(Emitter* emitter, const FuncFrame& frame, const FuncArgsAssignment& args);
};

//! \}
//! \endcond

ASMJIT_END_SUB_NAMESPACE

#endif // _ASMJIT_X86_X86INTERNAL_P_H
