#include "HackableCode.h"

HackableCode::HackableCode(void* codeStart, void* codeEnd)
{
	this->codePointer = (unsigned char*)codeStart;
	this->codeOriginalLength = (unsigned int)codeEnd - (unsigned int)codeStart;

	this->originalCodeCopy = new unsigned char(this->codeOriginalLength);
	memcpy(originalCodeCopy, codeStart, this->codeOriginalLength);

	this->assemblyString = this->disassemble(codeStart, this->codeOriginalLength);
}

void HackableCode::restoreOriginalCode()
{
	DWORD old;
	VirtualProtect(this->codePointer, this->codeOriginalLength, PAGE_EXECUTE_READWRITE, &old);
	memcpy(this->codePointer, this->originalCodeCopy, this->codeOriginalLength);
}

bool HackableCode::applyCustomCode()
{
	CompileResult compileResult = this->assemble(this->assemblyString, this->codePointer);

	if (compileResult.hasError)
	{
		std::cout << "Line #" << compileResult.errorData.lineNumber << ":" << "\n";
		std::cout << compileResult.errorData.message << "\n";

		// Fail the activation
		return false;
	}

	if (compileResult.byteCount > this->codeOriginalLength)
	{
		std::cout << "The new code is larger than the original! It doesn't fit." << "\n";

		// Fail the activation
		return false;
	}

	// Write new assembly code
	DWORD old;
	VirtualProtect(this->codePointer, compileResult.byteCount, PAGE_EXECUTE_READWRITE, &old);
	memcpy(this->codePointer, compileResult.compiledBytes, compileResult.byteCount);

	int unfilledBytes = this->codeOriginalLength - compileResult.byteCount;

	// Fill remaining bytes with NOPs
	for (int index = 0; index < unfilledBytes; index++)
	{
		const byte nop = 0x90;
		((byte*)this->codePointer)[compileResult.byteCount + index] = nop;
	}
}

HackableCode::~HackableCode()
{
	delete(this->originalCodeCopy);
}


HackableCode::CompileResult HackableCode::assemble(std::string assembly, void* addressStart)
{
	Fasm::FasmResult* fasmResult = Fasm::assemble(assembly, addressStart);
	CompileResult compileResult = this->constructCompileResult(fasmResult);

	delete(fasmResult);
	return compileResult;
}

std::string HackableCode::disassemble(void* bytes, int length)
{
	static ud_t ud_obj;
	static bool initialized = false;

	// Only initialize the disassembler once
	if (!initialized)
	{
		ud_init(&ud_obj);
		ud_set_mode(&ud_obj, sizeof(void*) * 8);
		ud_set_syntax(&ud_obj, UD_SYN_INTEL);
	}

	ud_set_input_buffer(&ud_obj, (unsigned char*)bytes, length);

	std::string instructions = "";

	while (ud_disassemble(&ud_obj))
	{
		instructions += std::string(ud_insn_asm(&ud_obj));
		instructions += "\n";
	}

	return instructions;
}



HackableCode::CompileResult HackableCode::constructCompileResult(Fasm::FasmResult* fasmResult)
{
	// Note the 2 here is due to the first two lines of FASM specifying 32/64 bit and the origin point
	const int lineOffset = 2;

	CompileResult compileResult;


	if (fasmResult->ErrorLine != nullptr)
	{
		compileResult.errorData.lineNumber = fasmResult->ErrorLine->LineNumber - lineOffset;
	}
	else
	{
		compileResult.errorData.lineNumber = 0;
	}

	switch (fasmResult->Error)
	{
	case Fasm::FasmErrorCode::FileNotFound:
		compileResult.errorData.message = "File Not Found";
		break;
	case Fasm::FasmErrorCode::ErrorReadingFile:
		compileResult.errorData.message = "Error Reading File";
		break;
	case Fasm::FasmErrorCode::InvalidFileFormat:
		compileResult.errorData.message = "Invalid File Format";
		break;
	case Fasm::FasmErrorCode::InvalidMacroArguments:
		compileResult.errorData.message = "Invalid Macro Arguments";
		break;
	case Fasm::FasmErrorCode::IncompleteMacro:
		compileResult.errorData.message = "Incomplete Macro";
		break;
	case Fasm::FasmErrorCode::UnexpectedCharacters:
		compileResult.errorData.message = "Unexpected Characters";
		break;
	case Fasm::FasmErrorCode::InvalidArgument:
		compileResult.errorData.message = "Invalid Argument";
		break;
	case Fasm::FasmErrorCode::IllegalInstruction:
		compileResult.errorData.message = "Illegal Instruction";
		break;
	case Fasm::FasmErrorCode::InvalidOperand:
		compileResult.errorData.message = "Invalid Operand";
		break;
	case Fasm::FasmErrorCode::InvalidOperandSize:
		compileResult.errorData.message = "Invalid Operand Size";
		break;
	case Fasm::FasmErrorCode::OperandSizeNotSpecified:
		compileResult.errorData.message = "Operand Size Not Specified";
		break;
	case Fasm::FasmErrorCode::OperandSizesDoNotMatch:
		compileResult.errorData.message = "Operand Sizes Do Not Match";
		break;
	case Fasm::FasmErrorCode::InvalidAddressSize:
		compileResult.errorData.message = "Invalid Address Size";
		break;
	case Fasm::FasmErrorCode::AddressSizesDoNotAgree:
		compileResult.errorData.message = "Address Sizes Do Not Agree";
		break;
	case Fasm::FasmErrorCode::DisallowedCombinationOfRegisters:
		compileResult.errorData.message = "Disallowed Combination Of Registers";
		break;
	case Fasm::FasmErrorCode::LongImmediateNotEncodable:
		compileResult.errorData.message = "Long Immediate Not Encodable";
		break;
	case Fasm::FasmErrorCode::RelativeJumpOutOfRange:
		compileResult.errorData.message = "Relative Jump Out Of Range";
		break;
	case Fasm::FasmErrorCode::InvalidExpression:
		compileResult.errorData.message = "Invalid Expression";
		break;
	case Fasm::FasmErrorCode::InvalidAddress:
		compileResult.errorData.message = "Invalid Address";
		break;
	case Fasm::FasmErrorCode::InvalidValue:
		compileResult.errorData.message = "Invalid Value";
		break;
	case Fasm::FasmErrorCode::ValueOutOfRange:
		compileResult.errorData.message = "Value Out Of Range";
		break;
	case Fasm::FasmErrorCode::UndefinedSymbol:
		compileResult.errorData.message = "Undefined Symbol";
		break;
	case Fasm::FasmErrorCode::InvalidUseOfSymbol:
		compileResult.errorData.message = "Invalid Use Of Symbol";
		break;
	case Fasm::FasmErrorCode::NameTooLong:
		compileResult.errorData.message = "Name Too Long";
		break;
	case Fasm::FasmErrorCode::InvalidName:
		compileResult.errorData.message = "Invalid Name";
		break;
	case Fasm::FasmErrorCode::ReservedWordUsedAsSymbol:
		compileResult.errorData.message = "Reserved Word Used As Symbol";
		break;
	case Fasm::FasmErrorCode::SymbolAlreadyDefined:
		compileResult.errorData.message = "Symbol Already Defined";
		break;
	case Fasm::FasmErrorCode::MissingEndQuote:
		compileResult.errorData.message = "Missing End Quote";
		break;
	case Fasm::FasmErrorCode::MissingEndDirective:
		compileResult.errorData.message = "Missing End Directive";
		break;
	case Fasm::FasmErrorCode::UnexpectedInstruction:
		compileResult.errorData.message = "Unexpected Instruction";
		break;
	case Fasm::FasmErrorCode::ExtraCharactersOnLine:
		compileResult.errorData.message = "Extra Characters On Line";
		break;
	case Fasm::FasmErrorCode::SectionNotAlignedEnough:
		compileResult.errorData.message = "Section Not Aligned Enough";
		break;
	case Fasm::FasmErrorCode::SettingAlreadySpecified:
		compileResult.errorData.message = "Setting Already Specified";
		break;
	case Fasm::FasmErrorCode::DataAlreadyDefined:
		compileResult.errorData.message = "Data Already Defined";
		break;
	case Fasm::FasmErrorCode::TooManyRepeats:
		compileResult.errorData.message = "Too Many Repeats";
		break;
	case Fasm::FasmErrorCode::SymbolOutOfScope:
		compileResult.errorData.message = "Symbol Out Of Scope";
		break;
	case Fasm::FasmErrorCode::UserError:
		compileResult.errorData.message = "User Error";
		break;
	case Fasm::FasmErrorCode::AssertionFailed:
		compileResult.errorData.message = "Assertion Failed";
		break;
	default:
		compileResult.errorData.message = "";
		break;
	}

	switch (fasmResult->Condition)
	{
	case Fasm::FasmResultCode::CannotGenerateCode:
	case Fasm::FasmResultCode::Error:
	case Fasm::FasmResultCode::FormatLimitationsExcedded:
	case Fasm::FasmResultCode::InvalidParameter:
	case Fasm::FasmResultCode::OutOfMemory:
	case Fasm::FasmResultCode::SourceNotFound:
	case Fasm::FasmResultCode::StackOverflow:
	case Fasm::FasmResultCode::UnexpectedEndOfSource:
	case Fasm::FasmResultCode::Working:
	case Fasm::FasmResultCode::WriteFailed:
	{
		compileResult.hasError = true;
		break;
	}
	case Fasm::FasmResultCode::Ok:
	default:
		compileResult.byteCount = fasmResult->OutputLength;
		compileResult.compiledBytes = new unsigned char[fasmResult->OutputLength];
		memcpy(compileResult.compiledBytes, fasmResult->OutputData, fasmResult->OutputLength);
		compileResult.hasError = false;
		break;
	}


	return compileResult;
}
