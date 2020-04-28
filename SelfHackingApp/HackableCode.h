#pragma once
#include <map>
#include <string>
#include <vector>

#ifndef _WIN32
#include <sys/mman.h>
#endif

// These allow for MACRO overloading
#define GET_MACRO(_1,_2,_3,NAME,...) NAME
#define ASM(...) GET_MACRO(__VA_ARGS__, ASM3, ASM2, ASM1)(__VA_ARGS__)

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

#define COMMENT(str) Strings::Common_ConstantNewline::create()->setStringReplacementVariables( \
	Strings::Common_Comment::create()->setStringReplacementVariables(str))->getString()

#if (_WIN64 || (__GNUC__ && (__x86_64__ || __ppc64__)))
#define ZAX rax
#define ZBX rbx
#define ZCX rcx
#define ZDX rdx
#define ZSI rsi
#define ZDI rdi
#define ZBP rbp
#define ZSP rsp
#define DIV_CONVERT cqo
#else
#define ZAX eax
#define ZBX ebx
#define ZCX ecx
#define ZDX edx
#define ZSI esi
#define ZDI edi
#define ZBP ebp
#define ZSP esp
#define DIV_CONVERT cdq
#endif

// Define macros for inlining x86 assembly in a compiler-independent way
#ifdef _MSC_VER
#define NO_OPTIMIZE \
		__pragma(optimize("", off))
#define END_NO_OPTIMIZE \
		__pragma(optimize("", on))
#define ASM1(asm_literal) \
		__asm asm_literal
#define ASM2(asm_literal1, asm_literal2) \
		__asm asm_literal1, asm_literal2
#define ASM3(asm_literal1, asm_literal2, asm_literal3) \
		__asm asm_literal1, asm_literal2, asm_literal3

#define ASM_MOV_REG_VAR(register, variable) \
		ASM(mov register, variable)

#define ASM_MOV_VAR_REG(variable, register) \
		ASM(mov variable, register)

#elif __GNUC__ || __clang__
#ifdef __clang__
#define NO_OPTIMIZE \
			__attribute__((optnone))
#define END_NO_OPTIMIZE
#elif __GNUC__
#define NO_OPTIMIZE \
			_Pragma("GCC push_options") \
			_Pragma("GCC optimize (\"O0\")") \
			 __attribute__((optimize("O0")))
#define END_NO_OPTIMIZE \
			_Pragma("GCC pop_options")
#endif

#define ASM1(asm_literal) \
		ASM_GCC(#asm_literal)
#define ASM2(asm_literal1, asm_literal2) \
		ASM_GCC(#asm_literal1 ", " #asm_literal2)
#define ASM3(asm_literal1, asm_literal2, asm_literal3) \
		ASM_GCC(#asm_literal1 ", " #asm_literal2 ", " #asm_literal3)

#ifdef __x86_64__
#define ASM_MOV_REG_VAR(register, variable) \
			__asm__ __volatile__("movq %0, %%" EXPAND_AND_QUOTE(register)  : /* no outputs */ : "m"(variable) : )

#define ASM_MOV_VAR_REG(variable, register) \
			__asm__ __volatile__("movq %%" EXPAND_AND_QUOTE(register) ", %0"  : "=m"(variable) : /* no inputs */ : )
#else
#define ASM_MOV_REG_VAR(register, variable) \
			__asm__ __volatile__("mov %0, %%" EXPAND_AND_QUOTE(register)  : /* no outputs */ : "m"(variable) : )

#define ASM_MOV_VAR_REG(variable, register) \
			__asm__ __volatile__("mov %%" EXPAND_AND_QUOTE(register) ", %0"  : "=m"(variable) : /* no inputs */ : )
#endif

#define ASM_GCC(asm_string) \
		__asm__ __volatile__(".intel_syntax noprefix;" asm_string ";.att_syntax prefix"); \

#endif

// This is used to mark the beginning of an editable section of code
// 56 6A 45 BE DE C0 ED FE 5E 5E
#define HACKABLE_CODE_BEGIN() \
	ASM(push ZDI) \
	ASM(push 69) \
	ASM(mov edi, 0xFEEDC0DE) \
	ASM(pop ZDI) \
	ASM(pop ZDI)

// This is used to mark the end of an editable section of code
// 56 6A 45 BE DE C0 AD DE 5E 5E
#define HACKABLE_CODE_END() \
	ASM(push ZSI) \
	ASM(push 69) \
	ASM(mov esi, 0xDEADC0DE) \
	ASM(pop ZSI) \
	ASM(pop ZSI)

// This is used to stop searching for hackable sections
// 56 6A 45 BE 5E EA 5E D1 5E 5E
#define HACKABLES_STOP_SEARCH() \
	ASM(push ZDX) \
	ASM(push 69) \
	ASM(mov edx, 0x0D15EA5E) \
	ASM(pop ZDX) \
	ASM(pop ZDX)

#define ASM_NOP1() ASM(nop)
#define ASM_NOP2() ASM_NOP1() ASM_NOP1()
#define ASM_NOP3() ASM_NOP2() ASM_NOP1()
#define ASM_NOP4() ASM_NOP3() ASM_NOP1()
#define ASM_NOP5() ASM_NOP4() ASM_NOP1()
#define ASM_NOP6() ASM_NOP5() ASM_NOP1()
#define ASM_NOP7() ASM_NOP6() ASM_NOP1()
#define ASM_NOP8() ASM_NOP7() ASM_NOP1()
#define ASM_NOP9() ASM_NOP8() ASM_NOP1()
#define ASM_NOP10() ASM_NOP9() ASM_NOP1()
#define ASM_NOP11() ASM_NOP10() ASM_NOP1()
#define ASM_NOP12() ASM_NOP11() ASM_NOP1()
#define ASM_NOP13() ASM_NOP12() ASM_NOP1()
#define ASM_NOP14() ASM_NOP13() ASM_NOP1()
#define ASM_NOP15() ASM_NOP14() ASM_NOP1()
#define ASM_NOP16() ASM_NOP15() ASM_NOP1()

class HackableCode
{
public:
	static std::vector<HackableCode*> create(void* functionStart);

	std::string getAssemblyString();
	std::string getOriginalAssemblyString();
	void* getPointer();
	int getOriginalLength();
	bool applyCustomCode(std::string newAssembly);
	void restoreState();

protected:
	HackableCode(void* codeStart, void* codeEnd);
	virtual ~HackableCode();

private:
	struct HackableCodeMarkers
	{
		void* start;
		void* end;

		HackableCodeMarkers() : start(nullptr), end(nullptr) { }
		HackableCodeMarkers(void* start, void* end) : start(start), end(end) { }
	};

	typedef std::map<void*, std::vector<HackableCode::HackableCodeMarkers>> MarkerMap;

	static std::vector<HackableCode*> parseHackables(void* functionStart);
	static std::vector<HackableCode::HackableCodeMarkers>& parseHackableMarkers(void* functionStart);

	std::string assemblyString;
	std::string originalAssemblyString;
	void* codePointer;
	void* codeEndPointer;
	std::vector<unsigned char> originalCodeCopy;
	int originalCodeLength;

	static MarkerMap MarkerCache;
	static const unsigned char StartTagSignature[];
	static const unsigned char EndTagSignature[];
	static const unsigned char StopSearchTagSignature[];
};
