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

	// [Decision] - who lets the player in. Speech values are the player's Speech skill (0 to 100).
	inline int friendRank = 1;            // iFriendRank - relationship rank from which a knock is always answered: 1 friend .. 4 lover
	inline int knockSpeech = 50;          // iKnockSpeech - Speech a plain Knock needs from a stranger, before the penalties
	inline int persuadeSpeech = 30;       // iPersuadeSpeech - Speech Persuade needs, before the same penalties
	inline int nightPenalty = 15;         // iNightPenalty - added at night
	inline int asleepPenalty = 15;        // iAsleepPenalty - added when the one answering was asleep
	inline int bountyGold = 1;            // iBountyGold - a bounty of this much or more in their hold counts (0 = ignore bounties)
	inline int bountyPenalty = 25;        // iBountyPenalty
	inline int thiefGold = 1;             // iThiefGold - seen stealing goods worth this much or more in their hold (0 = ignore)
	inline int thiefPenalty = 20;         // iThiefPenalty
	inline int dislikePenalty = 15;       // iDislikePenalty - for each rank below acquaintance (rival 1, foe 2, enemy 3, archnemesis 4)
	inline int intimidateMargin = 0;      // iIntimidateMargin - how far the player's intimidation must beat theirs
	inline int retryHours = 1;            // iRetryHours - game hours after a refusal before a new Knock starts over

	// [Voice] - the vanilla topics spoken in the resident's voice. "Plugin|0xID". Not on the page, by design.
	inline std::string letInTopic = "Skyrim.esm|0x039C2A";    // DialogueGeneric, subtype Trespass Against NPC: "I'll let you stay. This time."
	inline std::string refuseTopic = "Skyrim.esm|0x06AEA2";   // DialogueGeneric, subtype Time To Go: "You need to leave."
	inline int voiceVolume = 100;         // iVoiceVolume:Voice - 0 to 100, how loud the answer through the door is, in percent of the game's own loudness. On the page.

	// [Sound] - the knock itself. iKnockSound (on the page): 0 = off, 1 = sharp only (our own loose file, the last 3.5 s of the
	// vanilla knock: the two knocks without the stone grinding before them), 2 = full (the vanilla descriptor below),
	// 3 = wood (the player plays IdleMT_DoorBang and a vanilla wood footstep knocks five times; kialiva's idea and timings).
	enum KnockSoundMode : int { kKnockOff = 0, kKnockSharp = 1, kKnockFull = 2, kKnockWood = 3 };
	inline int knockSoundMode = kKnockWood;   // iKnockSound:Sound - wood by default (Main Agent's order 2, 2026-11-09)
	// sKnockSound - the vanilla sound descriptor "Plugin|0xID" that "full" plays; empty = no sound in "full". Not on the page.
	inline std::string knockSound = "Skyrim.esm|0x10080A";   // DRSStoneLabyrinthianKnock, Sound\FX\DRS\Labyrinthian\DRS_Stone_Labyrinthian_Knock.wav
	// The "sharp only" file, shipped loose with the mod, relative to Data.
	inline constexpr const char* kKnockSharpFile = "Sound\\FX\\DynamicSimpleKnock\\DSK_Knock_Sharp.wav";

	// [Exclusions] - every value of the section: "Plugin.esp|0x123456" or an editor ID. Not on the page, by design.
	inline std::vector<std::string> exclusions;

	void Init(const char* a_iniFileName);
	bool Reload();
	bool Save();
	void RestoreDefaults();
	void ApplyLogLevel();
	const std::string& GetIniPath();
}
