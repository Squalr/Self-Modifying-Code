#include "HackableCode.h"

#include <iostream>

#include "HackUtils.h"

HackableCode::CodeMap HackableCode::HackableCodeCache = HackableCode::CodeMap();

// Note: all tags are assumed to start with a different byte and have the same length
const int HackableCode::StartTagFuncIdIndex = 2;
const unsigned char HackableCode::StartTagSignature[] = { 0x57, 0x6A, 0x00, 0xBF, 0xDE, 0xC0, 0xED, 0xFE, 0x5F, 0x5F };
const unsigned char HackableCode::EndTagSignature[] = { 0x56, 0x6A, 0x45, 0xBE, 0xDE, 0xC0, 0xAD, 0xDE, 0x5E, 0x5E };
const unsigned char HackableCode::StopSearchTagSignature[] = { 0x52, 0x6A, 0x45, 0xBA, 0x5E, 0xEA, 0x15, 0x0D, 0x5A, 0x5A };
std::map<std::string, std::vector<unsigned char>> HackableCode::OriginalCodeCache = std::map<std::string, std::vector<unsigned char>>();

std::vector<HackableCode*> HackableCode::create(void* functionStart)
{
	return HackableCode::parseHackables(functionStart);
}

HackableCode::HackableCode(void* codeStart, void* codeEnd)
{
	this->codePointer = (unsigned char*)codeStart;
	this->codeEndPointer = (unsigned char*)codeEnd;
	this->originalCodeLength = (int)((unsigned long)codeEnd - (unsigned long)codeStart);
	this->originalCodeCopy = std::vector<unsigned char>();
	this->originalAssemblyString = HackUtils::disassemble(codeStart, this->originalCodeLength);
	this->assemblyString = this->originalAssemblyString;
}

HackableCode::~HackableCode()
{
}

std::string HackableCode::getAssemblyString()
{
	return this->assemblyString;
}

std::string HackableCode::getOriginalAssemblyString()
{
	return this->originalAssemblyString;
}

void* HackableCode::getPointer()
{
	return this->codePointer;
}

int HackableCode::getOriginalLength()
{
	return this->originalCodeLength;
}

bool HackableCode::applyCustomCode(std::string newAssembly)
{
	this->assemblyString = newAssembly;

	if (this->codePointer == nullptr)
	{
		return false;
	}

	HackUtils::CompileResult compileResult = HackUtils::assemble(this->assemblyString, this->codePointer);

	// Try to compile code
	if (compileResult.hasError || compileResult.byteCount > this->originalCodeLength)
	{
		std::cout << compileResult.errorData.message << std::endl;

		// Fail the activation
		return false;
	}

	int unfilledBytes = this->originalCodeLength - compileResult.byteCount;

	// Fill remaining bytes with NOPs
	for (int index = 0; index < unfilledBytes; index++)
	{
		const unsigned char nop = 0x90;
		compileResult.compiledBytes.push_back(nop);
	}

	HackUtils::writeMemory(this->codePointer, compileResult.compiledBytes.data(), compileResult.compiledBytes.size());

	return true;
}

void HackableCode::restoreState()
{
	HackUtils::writeMemory(this->codePointer, this->originalCodeCopy.data(), this->originalCodeCopy.size());
}

std::vector<HackableCode*> HackableCode::parseHackables(void* functionStart)
{
	// Parse the HACKABLE_CODE_BEGIN/END pairs from the function. There may be multiple.
	MarkerMap markerMap = HackableCode::parseHackableMarkers(functionStart);

	std::vector<HackableCode*> extractedHackableCode = std::vector<HackableCode*>();

	// Bind the code info to each of the BEGIN/END markers to create a HackableCode object.
	for (auto marker : markerMap)
	{
		unsigned char funcId = marker.first;
		HackableCodeMarkers markers = marker.second;

		extractedHackableCode.push_back(new HackableCode(markers.start, markers.end));
	}

	return extractedHackableCode;
}

HackableCode::MarkerMap& HackableCode::parseHackableMarkers(void* functionStart)
{
	if (HackableCode::HackableCodeCache.find(functionStart) != HackableCode::HackableCodeCache.end())
	{
		return HackableCode::HackableCodeCache[functionStart];
	}

	void* resolvedFunctionStart = HackUtils::resolveVTableAddress(functionStart);
	MarkerMap extractedMarkers = MarkerMap();

	const int tagSize = sizeof(HackableCode::StartTagSignature) / sizeof((HackableCode::StartTagSignature)[0]);
	const int stopSearchingAfterXBytesFailSafe = 4096;

	unsigned char* currentBase = (unsigned char*)resolvedFunctionStart;
	unsigned char* currentSeek = (unsigned char*)resolvedFunctionStart;
	unsigned char funcId = 0;
	const unsigned char* targetArray = nullptr;
	void* nextHackableCodeStart = nullptr;

	while (true)
	{
		int signatureIndex = (int)((unsigned long)currentSeek - (unsigned long)currentBase);

		if (signatureIndex > stopSearchingAfterXBytesFailSafe)
		{
			std::cout << "Potentially fatal error: unable to find end signature in hackable code!" << std::endl;
			break;
		}

		if (targetArray == nullptr)
		{
			if (*currentSeek == HackableCode::StartTagSignature[0])
			{
				targetArray = HackableCode::StartTagSignature;
			}
			else if (*currentSeek == HackableCode::EndTagSignature[0])
			{
				targetArray = HackableCode::EndTagSignature;
			}
			else if (*currentSeek == HackableCode::StopSearchTagSignature[0])
			{
				targetArray = HackableCode::StopSearchTagSignature;
			}
			else
			{
				// Next byte does not match the start of any signature
				currentBase++;
				currentSeek = currentBase;
				continue;
			}

			currentSeek++;
			continue;
		}

		// Special case in the start tag where we embed a local identifier for the function, which we need to pull out
		if (targetArray == HackableCode::StartTagSignature && signatureIndex == HackableCode::StartTagFuncIdIndex)
		{
			funcId = *currentSeek;
			currentSeek++;
			continue;
		}

		// Check if we match the next expected character
		if (*currentSeek == targetArray[signatureIndex])
		{
			currentSeek++;

			if (signatureIndex == tagSize - 1)
			{
				if (targetArray == HackableCode::StartTagSignature)
				{
					nextHackableCodeStart = (void*)currentSeek;
				}
				else if (targetArray == HackableCode::EndTagSignature)
				{
					if (nextHackableCodeStart != nullptr)
					{
						void* nextHackableCodeEnd = (void*)currentBase;

						extractedMarkers[funcId] = HackableCodeMarkers(nextHackableCodeStart, nextHackableCodeEnd);

						nextHackableCodeStart = nullptr;
					}
				}
				else if (targetArray == HackableCode::StopSearchTagSignature)
				{
					break;
				}
			}
			else
			{
				// Keep searching this signature
				continue;
			}
		}

		// Reset search state
		targetArray = nullptr;
		currentBase++;
		currentSeek = currentBase;
	}

	HackableCode::HackableCodeCache[functionStart] = extractedMarkers;

	return HackableCode::HackableCodeCache[functionStart];
}
