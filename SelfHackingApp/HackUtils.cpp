#include "HackUtils.h"

#include <algorithm>
#include <bitset>
#include <iomanip>
#include <regex>
#include <sstream>

#if __GNUC__ || __clang__
#include <unistd.h>
#include <sys/mman.h>
#endif

#include "StrUtils.h"
#include "External/asmjit/asmjit.h"
#include "External/asmtk/asmtk.h"
#include "External/libudis86/udis86.h"

using namespace asmjit;
using namespace asmtk;

void HackUtils::setAllMemoryPermissions(void* address, int length)
{
#ifdef _WIN32
	DWORD old;
	VirtualProtect(address, length, PAGE_EXECUTE_READWRITE, &old);
#else
	// Unix requires changing memory protection on the start of the page, not just the address
	size_t pageSize = sysconf(_SC_PAGESIZE);
	void* pageStart = (void*)((unsigned long)address & -pageSize);
	int newLength = (int)((unsigned long)address + length - (unsigned long)pageStart);

	mprotect(pageStart, newLength, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif
}

void HackUtils::writeMemory(void* to, void* from, int length)
{
	HackUtils::setAllMemoryPermissions(to, length);
	HackUtils::setAllMemoryPermissions(from, length);

	memcpy(to, from, length);
}

std::string HackUtils::preProcessAssembly(std::string assembly)
{
	std::string processedAssembly = "";

	auto floatParser = [&](std::string const& match)
	{
		std::string matchTrimmed = StrUtils::rtrim(match, "f");
		std::istringstream iss(matchTrimmed);
		float parsedFloat;

		if (iss >> parsedFloat)
		{
			int floatAsRawIntBytes = *(int*)(&parsedFloat);
			processedAssembly += HackUtils::toHex(floatAsRawIntBytes, true);
		}
		else
		{
			processedAssembly += match;
		}
	};

	std::regex reg("[-]?[0-9]*\\.[0-9]+f");
	std::sregex_token_iterator begin(assembly.begin(), assembly.end(), reg, { -1, 0 }), end;
	std::for_each(begin, end, floatParser);

	// Convert to normalized comment formats
	processedAssembly = StrUtils::replaceAll(processedAssembly, "//", ";");

	return processedAssembly;
}

HackUtils::CompileResult HackUtils::assemble(std::string assembly, void* addressStart)
{
	CompileResult compileResult;

	CodeInfo ci(sizeof(void*) == 4 ? ArchInfo::kIdX86 : ArchInfo::kIdX64);
	CodeHolder code;
	code.init(ci);

	// Attach X86Assembler `code`.
	x86::Assembler a(&code);

	// Create AsmParser that will emit to X86Assembler.
	AsmParser p(&a);

	// Parse the assembly.
	assembly = preProcessAssembly(assembly);
	Error err = p.parse(assembly.c_str());

	// Error handling (use asmjit::ErrorHandler for more robust error handling).
	if (err)
	{
		compileResult.hasError = true;
		compileResult.errorData.lineNumber = 0;

		switch ((CompileResult::ErrorId) err)
		{
		case CompileResult::ErrorId::Ok:
		{
			compileResult.errorData.message = "OK";
			break;
		}
		case CompileResult::ErrorId::NoHeapMemory:
		{
			compileResult.errorData.message = "No heap memory";
			break;
		}
		case CompileResult::ErrorId::NoVirtualMemory:
		{
			compileResult.errorData.message = "No virtual memory";
			break;
		}
		case CompileResult::ErrorId::InvalidArgument:
		{
			compileResult.errorData.message = "Invalid argument";
			break;
		}
		case CompileResult::ErrorId::InvalidState:
		{
			compileResult.errorData.message = "Invalid state";
			break;
		}
		case CompileResult::ErrorId::InvalidArchitecture:
		{
			compileResult.errorData.message = "Invalid architecture";
			break;
		}
		case CompileResult::ErrorId::NotInitialized:
		{
			compileResult.errorData.message = "Not initialized";
			break;
		}
		case CompileResult::ErrorId::AlreadyInitialized:
		{
			compileResult.errorData.message = "Already initialized";
			break;
		}
		case CompileResult::ErrorId::FeatureNotEnabled:
		{
			compileResult.errorData.message = "Feature not enabled";
			break;
		}
		case CompileResult::ErrorId::SlotOccupied:
		{
			compileResult.errorData.message = "Slot occupied";
			break;
		}
		case CompileResult::ErrorId::NoCodeGenerated:
		{
			compileResult.errorData.message = "No code generated";
			break;
		}
		case CompileResult::ErrorId::CodeTooLarge:
		{
			compileResult.errorData.message = "Code too large";
			break;
		}
		case CompileResult::ErrorId::InvalidLabel:
		{
			compileResult.errorData.message = "Invalid label";
			break;
		}
		case CompileResult::ErrorId::LabelIndexOverflow:
		{
			compileResult.errorData.message = "Label index overflow";
			break;
		}
		case CompileResult::ErrorId::LabelAlreadyBound:
		{
			compileResult.errorData.message = "Label already bound";
			break;
		}
		case CompileResult::ErrorId::LabelAlreadyDefined:
		{
			compileResult.errorData.message = "Label already defined";
			break;
		}
		case CompileResult::ErrorId::LabelNameTooLong:
		{
			compileResult.errorData.message = "Label name too long";
			break;
		}
		case CompileResult::ErrorId::InvalidLabelName:
		{
			compileResult.errorData.message = "Invalid label name";
			break;
		}
		case CompileResult::ErrorId::InvalidParentLabel:
		{
			compileResult.errorData.message = "Invalid parent label";
			break;
		}
		case CompileResult::ErrorId::NonLocalLabelCantHaveParent:
		{
			compileResult.errorData.message = "Non local label can't have parent";
			break;
		}
		case CompileResult::ErrorId::RelocationIndexOverflow:
		{
			compileResult.errorData.message = "Relocation index overflow";
			break;
		}
		case CompileResult::ErrorId::InvalidRelocationEntry:
		{
			compileResult.errorData.message = "Invalid relocation entry";
			break;
		}
		case CompileResult::ErrorId::InvalidInstruction:
		{
			compileResult.errorData.message = "Invalid instruction";
			break;
		}
		case CompileResult::ErrorId::InvalidRegisterType:
		{
			compileResult.errorData.message = "Invalid register type";
			break;
		}
		case CompileResult::ErrorId::InvalidRegisterKind:
		{
			compileResult.errorData.message = "Invalid register kind";
			break;
		}
		case CompileResult::ErrorId::InvalidRegisterPhysicalId:
		{
			compileResult.errorData.message = "Invalid physical id";
			break;
		}
		case CompileResult::ErrorId::InvalidRegisterVirtualId:
		{
			compileResult.errorData.message = "Invalid register virutal id";
			break;
		}
		case CompileResult::ErrorId::InvalidPrefixCombination:
		{
			compileResult.errorData.message = "Invalid prefix combination";
			break;
		}
		case CompileResult::ErrorId::InvalidLockPrefix:
		{
			compileResult.errorData.message = "Invalid lock prefix";
			break;
		}
		case CompileResult::ErrorId::InvalidXAcquirePrefix:
		{
			compileResult.errorData.message = "Invalid x acquire prefix";
			break;
		}
		case CompileResult::ErrorId::InvalidXReleasePrefix:
		{
			compileResult.errorData.message = "Invalid x release prefix";
			break;
		}
		case CompileResult::ErrorId::InvalidRepPrefix:
		{
			compileResult.errorData.message = "Invalid rep prefix";
			break;
		}
		case CompileResult::ErrorId::InvalidRexPrefix:
		{
			compileResult.errorData.message = "Invalid rex prefix";
			break;
		}
		case CompileResult::ErrorId::InvalidMask:
		{
			compileResult.errorData.message = "Invalid mask";
			break;
		}
		case CompileResult::ErrorId::InvalidUseSingle:
		{
			compileResult.errorData.message = "Invalid use single";
			break;
		}
		case CompileResult::ErrorId::InvalidUseDouble:
		{
			compileResult.errorData.message = "Invalid use double";
			break;
		}
		case CompileResult::ErrorId::InvalidBroadcast:
		{
			compileResult.errorData.message = "Invalid broadcast";
			break;
		}
		case CompileResult::ErrorId::InvalidOption:
		{
			compileResult.errorData.message = "Invalid option";
			break;
		}
		case CompileResult::ErrorId::InvalidAddress:
		{
			compileResult.errorData.message = "Invalid address";
			break;
		}
		case CompileResult::ErrorId::InvalidAddressIndex:
		{
			compileResult.errorData.message = "Invalid address index";
			break;
		}
		case CompileResult::ErrorId::InvalidAddressScale:
		{
			compileResult.errorData.message = "Invalid address scale";
			break;
		}
		case CompileResult::ErrorId::InvalidUseOf64BitAddress:
		{
			compileResult.errorData.message = "Invalid use of 64 bit address";
			break;
		}
		case CompileResult::ErrorId::InvalidDisplacement:
		{
			compileResult.errorData.message = "Invalid displacement";
			break;
		}
		case CompileResult::ErrorId::InvalidSegment:
		{
			compileResult.errorData.message = "Invalid segment";
			break;
		}
		case CompileResult::ErrorId::InvalidImmediateValue:
		{
			compileResult.errorData.message = "Invalid immediate value";
			break;
		}
		case CompileResult::ErrorId::InvalidOperandSize:
		{
			compileResult.errorData.message = "Invalid operand size";
			break;
		}
		case CompileResult::ErrorId::AmbiguousOperandSize:
		{
			compileResult.errorData.message = "Ambiguous operand size";
			break;
		}
		case CompileResult::ErrorId::OperandSizeMismatch:
		{
			compileResult.errorData.message = "Operand size mismatch";
			break;
		}
		case CompileResult::ErrorId::InvalidTypeInfo:
		{
			compileResult.errorData.message = "Invalid type info";
			break;
		}
		case CompileResult::ErrorId::InvalidUseOf8BitRegister:
		{
			compileResult.errorData.message = "Invalud use of 8 bit register";
			break;
		}
		case CompileResult::ErrorId::InvalidUseOf64BitRegister:
		{
			compileResult.errorData.message = "Invalid use of 64 bit register";
			break;
		}
		case CompileResult::ErrorId::InvalidUseOf80BitFloat:
		{
			compileResult.errorData.message = "Invalid use of 80 bit float";
			break;
		}
		case CompileResult::ErrorId::NotConsecutiveRegisters:
		{
			compileResult.errorData.message = "Not consecutive registers";
			break;
		}
		case CompileResult::ErrorId::NoPhysicalRegisters:
		{
			compileResult.errorData.message = "No physical registers";
			break;
		}
		case CompileResult::ErrorId::OverlappedRegisters:
		{
			compileResult.errorData.message = "Overlapped registers";
			break;
		}
		case CompileResult::ErrorId::OverlappingRegisterAndArgsRegister:
		{
			compileResult.errorData.message = "Overlapping register and args register";
			break;
		}
		case CompileResult::ErrorId::UnknownError:
		default:
		{
			compileResult.errorData.message = "Unknown error";
			break;
		}
		}

		return compileResult;
	}

	// Now you can print the code, which is stored in the first section (.text).
	CodeBuffer& buffer = code.sectionById(0)->buffer();
	uint8_t* bufferData = buffer.data();

	compileResult.hasError = false;
	compileResult.byteCount = buffer.size();
	compileResult.compiledBytes = std::vector<unsigned char>();

	for (int index = 0; index < compileResult.byteCount; index++)
	{
		compileResult.compiledBytes.push_back(bufferData[index]);
	}

	return compileResult;
}

void* HackUtils::resolveVTableAddress(void* address)
{
	std::string firstInstruction = HackUtils::disassemble(address, 5);

	if (StrUtils::startsWith(firstInstruction, "jmp ", true))
	{
		std::string newAddressStr = StrUtils::ltrim(firstInstruction, "jmp ", true);
		newAddressStr = StrUtils::rtrim(newAddressStr, "\n", true);

		address = HackUtils::intToPointer(newAddressStr, address);
	}

	return address;
}

std::string HackUtils::disassemble(void* address, int length)
{
	static ud_t ud_obj;
	static bool initialized = false;

	if (address == nullptr)
	{
		return "nullptr";
	}

	if (length <= 0)
	{
		return "";
	}

	// Only initialize the disassembler once
	if (!initialized)
	{
		ud_init(&ud_obj);
		ud_set_mode(&ud_obj, sizeof(void*) * 8);
		ud_set_syntax(&ud_obj, UD_SYN_INTEL);

		initialized = true;
	}

	ud_set_pc(&ud_obj, (uint64_t)address);
	ud_set_input_buffer(&ud_obj, (unsigned char*)address, length);

	std::string instructions = "";

	while (ud_disassemble(&ud_obj))
	{
		instructions += ud_insn_asm(&ud_obj);
		instructions += "\n";
	}

	return HackUtils::preProcess(instructions);
}

std::string HackUtils::preProcess(std::string instructions)
{
	const std::string regExpStr = "0x([0-9]|[a-f]|[A-F])+";
	const std::regex regExp = std::regex(regExpStr);

	if (!StrUtils::isRegexSubMatch(instructions, regExpStr))
	{
		return instructions;
	}

	std::string result = "";
	std::sregex_token_iterator begin = std::sregex_token_iterator(instructions.begin(), instructions.end(), regExp, { -1, 0 });
	std::sregex_token_iterator end = std::sregex_token_iterator();

	std::for_each(begin, end, [&](std::string const& next)
		{
			std::istringstream iss(next);

			if (StrUtils::isHexNumber(next))
			{
				result += std::to_string(StrUtils::hexToInt(next));
			}
			else
			{
				result += next;
			}
		});

	return result;
}

std::string HackUtils::toHex(int value, bool prefix)
{
	std::stringstream stream;

	// Convert to hex
	stream << std::hex << (int)(value);
	std::string hexString = stream.str();

	// Convert to upper
	std::transform(hexString.begin(), hexString.end(), hexString.begin(), ::toupper);

	return prefix ? ("0x" + hexString) : hexString;
}

void* HackUtils::intToPointer(std::string intString, void* fallback)
{
	if (!StrUtils::isInteger(intString))
	{
		return fallback;
	}

	void* address;
	std::stringstream ss;

	ss << std::hex << std::stoull(intString);
	std::string str = ss.str();
	ss >> address;

	return address;
}
