#pragma once

#include "HandleMemory.hpp"
#include <iostream>
#include <vector>

template<typename T>
class Instance {
protected:
	Instance() {}
	~Instance() {}

	Instance(const Instance&) = delete;
	Instance& operator=(const Instance&) = delete;

	Instance(Instance&&) = delete;
	Instance& operator=(Instance&&) = delete;
public:
	static auto Get() {
		static T inst{};
		return &inst;
	}
};

namespace Context
{
	enum class Ressources
	{
		GOLD = 0,
		BUSTES,
		PORTRAITS,
		CONTRACTS,
		BLASONS,
		SHARDS,
		DUST,
		BLUEPRINTS,
		END,
	};

	std::ostream &operator <<(std::ostream &os, const Ressources &res) {
		switch (res) {
		case Ressources::GOLD: return os << "Gold";
		case Ressources::BUSTES: return os << "Bustes";
		case Ressources::PORTRAITS: return os << "Portraits";
		case Ressources::CONTRACTS: return os << "Contracts";
		case Ressources::BLASONS: return os << "Blasons";
		case Ressources::SHARDS: return os << "Shards";
		case Ressources::DUST: return os << "Dust";
		case Ressources::BLUEPRINTS: return os << "Blueprints";
		default: return os << "Unknown";
		}
	}

	std::string to_string(const Ressources &res) {
		switch (res) {
		case Ressources::GOLD: return "Gold";
		case Ressources::BUSTES: return "Bustes";
		case Ressources::PORTRAITS: return "Portraits";
		case Ressources::CONTRACTS: return "Contracts";
		case Ressources::BLASONS: return "Blasons";
		case Ressources::SHARDS: return "Shards";
		case Ressources::DUST: return "Dust";
		case Ressources::BLUEPRINTS: return "Blueprints";
		default: return "Unknown";
		}
	}

	const long staticOffset = 0x00D9EE38;
	const long ressourcesOffset = 0x48;
	const std::vector<uint32_t> goldOffsets = {0x0, 0x540, 0x8, 0x18, 0x20, 0x0};

	class Cheat final
	{
	public:
		Cheat(const wchar_t* processName) : m_currentRessource(Ressources::GOLD), m_ressourceAmount(0), m_hProc(INVALID_HANDLE_VALUE), m_goldAddress(0)
		{
			DWORD pid = handleMemory::getPidByName(processName);
			if (pid == 0)
			{
				std::cerr << "Process not found!" << std::endl;
				exit(1);
			}

			m_hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
			if (m_hProc == INVALID_HANDLE_VALUE)
			{
				std::cerr << "Failed to open process!" << std::endl;
				exit(1);
			}

			uintptr_t moduleBase = handleMemory::getModuleBaseAddress(pid, L"Darkest.exe");
			if (moduleBase == 0)
			{
				std::cerr << "Module not found!" << std::endl;
				CloseHandle(m_hProc);
				exit(1);
			}

			m_goldAddress = handleMemory::FindDMA(m_hProc, moduleBase + staticOffset, goldOffsets);
		}
		~Cheat()
		{
			if (m_hProc != INVALID_HANDLE_VALUE)
			{
				CloseHandle(m_hProc);
			}
		}

		void refreshRessourceAmount()
		{
			if (m_hProc == INVALID_HANDLE_VALUE)
			{
				std::cerr << "Process handle is invalid!" << std::endl;
				return;
			}
			if (!ReadProcessMemory(m_hProc, reinterpret_cast<LPCVOID>(m_goldAddress + (ressourcesOffset * static_cast<uint32_t>(m_currentRessource))), &m_ressourceAmount, sizeof(m_ressourceAmount), nullptr))
			{
				std::cerr << "Failed." << std::endl;
			}
		}

		[[nodiscard]] std::size_t getRessourceAmount() const
		{
			return m_ressourceAmount;
		}

		[[nodiscard]] uintptr_t getGoldAddress() const
		{
			return m_goldAddress;
		}

		[[nodiscard]] Ressources getCurrent() const
		{
			return m_currentRessource;
		}

		void setCurrent(Ressources ressource)
		{
			if (ressource >= Ressources::END)
			{
				std::cerr << "Invalid ressource!" << std::endl;
				return;
			}
			m_currentRessource = ressource;
		}

		void setRessourceAmount(std::size_t amount)
		{
			if (m_hProc == INVALID_HANDLE_VALUE)
			{
				std::cerr << "Process handle is invalid!" << std::endl;
				return;
			}
			if (!WriteProcessMemory(m_hProc, reinterpret_cast<LPVOID>(m_goldAddress + (ressourcesOffset * static_cast<uint32_t>(m_currentRessource))), &amount, sizeof(amount), nullptr))
			{
				std::cerr << "Failed." << std::endl;
			}
			m_ressourceAmount = amount;
		}

	private:
		HANDLE m_hProc;
		uintptr_t m_goldAddress;
		std::size_t m_ressourceAmount;
		Ressources m_currentRessource;
	};
}
