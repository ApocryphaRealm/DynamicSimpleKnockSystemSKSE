#include "PCH.h"

#include "Settings.h"

#include "utils/Logger.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

namespace settings
{
	namespace
	{
		std::string iniPath;

		std::string Lower(std::string a_s)
		{
			for (char& c : a_s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
			return a_s;
		}

		std::string Trim(const std::string& a_s)
		{
			const auto b = a_s.find_first_not_of(" \t\r\n");
			if (b == std::string::npos) { return {}; }
			const auto e = a_s.find_last_not_of(" \t\r\n");
			return a_s.substr(b, e - b + 1);
		}

		struct IntKey
		{
			const char* name;   // lower case, as the reader compares
			int* target;
			int lo;
			int hi;
		};

		// The [Decision] section, one row per key: the INI name (lower case), the value and its range.
		std::vector<IntKey>& DecisionKeys()
		{
			static std::vector<IntKey> keys = {
				{ "ifriendrank", &friendRank, 1, 4 }, { "iknockspeech", &knockSpeech, 0, 200 }, { "ipersuadespeech", &persuadeSpeech, 0, 200 },
				{ "inightpenalty", &nightPenalty, 0, 100 }, { "iasleeppenalty", &asleepPenalty, 0, 100 }, { "ibountygold", &bountyGold, 0, 100000 },
				{ "ibountypenalty", &bountyPenalty, 0, 100 }, { "ithiefgold", &thiefGold, 0, 100000 }, { "ithiefpenalty", &thiefPenalty, 0, 100 },
				{ "idislikepenalty", &dislikePenalty, 0, 100 }, { "iintimidatemargin", &intimidateMargin, -100, 100 }, { "iretryhours", &retryHours, 0, 72 }
			};
			return keys;
		}

		bool LoadFileValues()
		{
			std::ifstream in(iniPath);
			if (!in)
			{
				logger::warn("INI not found at {}; keeping the current values", iniPath);
				return false;
			}
			exclusions.clear();
			std::string line, section;
			while (std::getline(in, line))
			{
				const std::string t = Trim(line);
				if (t.empty() || t[0] == ';' || t[0] == '#') { continue; }
				if (t.front() == '[' && t.back() == ']') { section = Lower(t.substr(1, t.size() - 2)); continue; }
				const auto eq = t.find('=');
				if (eq == std::string::npos) { continue; }
				const std::string key = Lower(Trim(t.substr(0, eq)));
				const std::string value = Trim(t.substr(eq + 1));
				try
				{
					if (section == "exclusions") { if (!value.empty()) { exclusions.push_back(value); } }
					else if (section == "general" && key == "benabled") { enabled = Lower(value) == "1" || Lower(value) == "true"; }
					else if (section == "general" && key == "idaystarthour") { dayStartHour = std::clamp(std::stoi(value), 0, 24); }
					else if (section == "general" && key == "idayendhour") { dayEndHour = std::clamp(std::stoi(value), 0, 24); }
					else if (section == "debug" && key == "uloglevel") { logLevel = static_cast<std::uint32_t>(std::stoul(value, nullptr, 0)); }
					else if (section == "decision")
					{
						for (auto& [name, target, lo, hi] : DecisionKeys())
						{
							if (key == name) { *target = std::clamp(std::stoi(value), lo, hi); }
						}
					}
					else if (section == "voice" && key == "sletintopic") { letInTopic = value; }
					else if (section == "voice" && key == "srefusetopic") { refuseTopic = value; }
					else if (section == "voice" && key == "ivoicevolume") { voiceVolume = std::clamp(std::stoi(value), 0, 100); }
					else if (section == "sound" && key == "sknocksound") { knockSound = value; }
				}
				catch (...)
				{
					logger::warn("INI value \"{}\" for {} is not valid; keeping the current value", value, key);
				}
			}
			logger::info("settings read from {}: enabled={} day={}-{} exclusions={} logLevel={}", iniPath, enabled, dayStartHour, dayEndHour, exclusions.size(), logLevel);
			logger::info("decision: friendRank={} knockSpeech={} persuadeSpeech={} night+{} asleep+{} bounty>={}:+{} thief>={}:+{} dislike+{}/rank intimidateMargin={} retryHours={}",
				friendRank, knockSpeech, persuadeSpeech, nightPenalty, asleepPenalty, bountyGold, bountyPenalty, thiefGold, thiefPenalty, dislikePenalty, intimidateMargin, retryHours);
			logger::info("voice: let in {}, refuse {}, volume {}%; knock sound {}", letInTopic, refuseTopic, voiceVolume, knockSound.empty() ? "none" : knockSound);
			return true;
		}

		// Replaces the key's value in place, so comments and the exclusion list survive; adds the key when it is missing.
		void WriteKey(std::vector<std::string>& a_lines, const char* a_section, const char* a_key, const std::string& a_value)
		{
			const std::string wantSection = Lower(a_section);
			const std::string wantKey = Lower(a_key);
			std::string section;
			std::size_t lastInSection = std::string::npos;
			for (std::size_t i = 0; i < a_lines.size(); ++i)
			{
				const std::string t = Trim(a_lines[i]);
				if (!t.empty() && t.front() == '[' && t.back() == ']')
				{
					section = Lower(t.substr(1, t.size() - 2));
					if (section == wantSection) { lastInSection = i; }
					continue;
				}
				if (section != wantSection) { continue; }
				if (!t.empty()) { lastInSection = i; }
				const auto eq = t.find('=');
				if (eq != std::string::npos && t[0] != ';' && Lower(Trim(t.substr(0, eq))) == wantKey)
				{
					a_lines[i] = std::string(a_key) + "=" + a_value;
					return;
				}
			}
			const std::string newLine = std::string(a_key) + "=" + a_value;
			if (lastInSection != std::string::npos)
			{
				a_lines.insert(a_lines.begin() + static_cast<std::ptrdiff_t>(lastInSection + 1), newLine);
			}
			else
			{
				a_lines.push_back(std::string("[") + a_section + "]");
				a_lines.push_back(newLine);
			}
		}
	}

	void Init(const char* a_iniFileName)
	{
		iniPath = (std::filesystem::current_path() / "Data" / "SKSE" / "Plugins" / a_iniFileName).string();
		LoadFileValues();
	}

	bool Reload()
	{
		const bool ok = LoadFileValues();
		ApplyLogLevel();
		return ok;
	}

	bool Save()
	{
		std::vector<std::string> lines;
		{
			std::ifstream in(iniPath);
			std::string line;
			while (in && std::getline(in, line)) { lines.push_back(line); }
		}
		WriteKey(lines, "General", "bEnabled", enabled ? "1" : "0");
		WriteKey(lines, "General", "iDayStartHour", std::to_string(dayStartHour));
		WriteKey(lines, "General", "iDayEndHour", std::to_string(dayEndHour));
		WriteKey(lines, "Decision", "iFriendRank", std::to_string(friendRank));
		WriteKey(lines, "Decision", "iKnockSpeech", std::to_string(knockSpeech));
		WriteKey(lines, "Decision", "iPersuadeSpeech", std::to_string(persuadeSpeech));
		WriteKey(lines, "Decision", "iNightPenalty", std::to_string(nightPenalty));
		WriteKey(lines, "Decision", "iAsleepPenalty", std::to_string(asleepPenalty));
		WriteKey(lines, "Decision", "iBountyGold", std::to_string(bountyGold));
		WriteKey(lines, "Decision", "iBountyPenalty", std::to_string(bountyPenalty));
		WriteKey(lines, "Decision", "iThiefGold", std::to_string(thiefGold));
		WriteKey(lines, "Decision", "iThiefPenalty", std::to_string(thiefPenalty));
		WriteKey(lines, "Decision", "iDislikePenalty", std::to_string(dislikePenalty));
		WriteKey(lines, "Decision", "iIntimidateMargin", std::to_string(intimidateMargin));
		WriteKey(lines, "Decision", "iRetryHours", std::to_string(retryHours));
		WriteKey(lines, "Voice", "iVoiceVolume", std::to_string(voiceVolume));
		WriteKey(lines, "Debug", "uLogLevel", std::to_string(logLevel));

		std::ofstream out(iniPath, std::ios::trunc);
		if (!out)
		{
			logger::error("Save: could not open {} for writing", iniPath);
			return false;
		}
		for (const auto& line : lines) { out << line << '\n'; }
		logger::info("settings saved to {}", iniPath);
		return true;
	}

	void RestoreDefaults()
	{
		enabled = true;
		dayStartHour = 8;
		dayEndHour = 20;
		friendRank = 1;
		knockSpeech = 50;
		persuadeSpeech = 30;
		nightPenalty = 15;
		asleepPenalty = 15;
		bountyGold = 1;
		bountyPenalty = 25;
		thiefGold = 1;
		thiefPenalty = 20;
		dislikePenalty = 15;
		intimidateMargin = 0;
		retryHours = 1;
	}

	void ApplyLogLevel()
	{
		const auto lvl = static_cast<spdlog::level::level_enum>(std::clamp<std::uint32_t>(logLevel, 0u, 6u));
		SKSE::log::set_level(lvl, lvl);
	}

	const std::string& GetIniPath() { return iniPath; }
}
