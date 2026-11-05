#pragma once

// Dynamic Simple Knock System SKSE - settings. Plain-file INI (never the Win32 profile API, which MO2's VFS does not redirect).

#include <cstdint>
#include <string>
#include <vector>

namespace settings
{
	inline bool enabled = true;           // bEnabled:General
	inline int dayStartHour = 8;          // iDayStartHour:General - 0 to 24, the hour the day begins
	inline int dayEndHour = 20;           // iDayEndHour:General - 0 to 24, the hour the night begins
	inline std::uint32_t logLevel = 2;    // uLogLevel:Debug - 2 = info

	// [Exclusions] - every value of the section: "Plugin.esp|0x123456" or an editor ID. Not on the page, by design.
	inline std::vector<std::string> exclusions;

	void Init(const char* a_iniFileName);
	bool Reload();
	bool Save();
	void RestoreDefaults();
	void ApplyLogLevel();
	const std::string& GetIniPath();
}
