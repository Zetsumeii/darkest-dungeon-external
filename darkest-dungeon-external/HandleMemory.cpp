#include "HandleMemory.hpp"

DWORD handleMemory::getPidByName(const std::wstring& processName)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Failed to create snapshot." << std::endl;
		exit(1);
	}

	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);
	if (!Process32FirstW(hSnapshot, &pe32))
	{
		std::cerr << "Failed to retrieve first process." << std::endl;
		CloseHandle(hSnapshot);
		exit(1);
	}

	do {
		if (processName == pe32.szExeFile)
		{
			CloseHandle(hSnapshot);
			return pe32.th32ProcessID;
		}
	} while (Process32NextW(hSnapshot, &pe32));
	CloseHandle(hSnapshot);

	return 0;
}

uintptr_t handleMemory::getModuleBaseAddress(DWORD pid, const std::wstring& moduleName)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Failed to create module snapshot." << std::endl;
		exit(1);
	}
	MODULEENTRY32W me32;
	me32.dwSize = sizeof(MODULEENTRY32W);
	if (!Module32FirstW(hSnapshot, &me32))
	{
		std::cerr << "Failed to retrieve first module." << std::endl;
		CloseHandle(hSnapshot);
		exit(1);
	}
	do {
		if (moduleName == me32.szModule)
		{
			CloseHandle(hSnapshot);
			return reinterpret_cast<uintptr_t>(me32.modBaseAddr);
		}
	} while (Module32NextW(hSnapshot, &me32));
	CloseHandle(hSnapshot);
	return 0;
}

uintptr_t handleMemory::FindDMA(HANDLE hProc, uintptr_t ptr, std::vector<uint32_t> offsets)
{
	uintptr_t addr = ptr;

	for (size_t i = 0; i < offsets.size(); ++i)
	{
		ReadProcessMemory(hProc, (BYTE*)addr, &addr, sizeof(addr), 0);
		addr += offsets[i];
	}

	return addr;
}
