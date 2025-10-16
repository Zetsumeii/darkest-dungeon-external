#pragma once

#include <Windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <string>
#include <vector>

namespace handleMemory
{
	DWORD getPidByName(const std::wstring& processName);
	uintptr_t getModuleBaseAddress(DWORD pid, const std::wstring& moduleName);
	uintptr_t FindDMA(HANDLE hProc, uintptr_t ptr, std::vector<uint32_t> offsets);
}
