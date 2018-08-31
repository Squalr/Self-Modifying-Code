#include "windows.h"
#include "fasm.h"
#include "udis86.h"

#include <string>
#include <map>

#define HACKABLE_CODE(address, label) \
_asm \
{ \
	_asm mov address, offset label \
	_asm label: \
}

class HackableCode
{
public:
	HackableCode(void* codeStart, void* codeEnd);
	~HackableCode();

	std::string assemblyString;

	void restoreOriginalCode();
	bool applyCustomCode();

private:
	void* allocateMemory(int allocationSize);

	struct CompileResult
	{
		struct ErrorData
		{
			int lineNumber;
			std::string message;
		};

		ErrorData errorData;
		bool hasError;

		unsigned char* compiledBytes;
		int byteCount;
	};

	CompileResult assemble(std::string assembly, void* addressStart);
	std::string disassemble(void* bytes, int length);
	CompileResult constructCompileResult(Fasm::FasmResult* fasmResult);

	void* codePointer;
	void* originalCodeCopy;
	int codeOriginalLength;
	std::map<void*, int>* allocations;
};
