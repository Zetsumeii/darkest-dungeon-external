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

	template <typename T>
	[[nodiscard]] T readMemory(HANDLE hProc, uintptr_t address)
	{
		T buffer;
		if (!ReadProcessMemory(hProc, reinterpret_cast<LPCVOID>(address), &buffer, sizeof(T), nullptr))
		{
			std::cerr << "Failed to read memory at address: " << std::hex << address << std::dec << std::endl;
			return T();
		}
		return buffer;
	}

	template <typename T>
	bool writeMemory(HANDLE hProc, uintptr_t address, T value)
	{
		if (!WriteProcessMemory(hProc, reinterpret_cast<LPVOID>(address), &value, sizeof(T), nullptr))
		{
			std::cerr << "Failed to write memory at address: " << std::hex << address << std::dec << std::endl;
			return false;
		}
		return true;
	}
}
