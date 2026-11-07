// Dynamic Simple Knock System SKSE - the rule and the two door hooks.
//
// THE RULE (Classify). A locked door, standing in an exterior cell, that teleports to an interior, is somebody's home when the
// door, its other half or the destination cell has an owner, or a living resident is placed in that cell. Left alone: the
// player sneaking or in combat, the player holding the key, the player's own home, the [Exclusions] list, a destination
// location with a dungeon keyword, anybody there hostile to the player. Interior-to-interior doors are left alone on purpose:
// the census shows they are mostly jarls' quarters, temple sanctums and vaults, and nobody knocks from inside the building.
//
// WHO ANSWERS (the owner, 2026-11-05 and 2026-11-07). Only somebody in the destination cell right now: a living, enabled actor
// whose current cell is that cell. Nobody there: no voice, no let-in, "No one's home. Come back later." The actors are read from the forms in memory, so while the cell is unloaded
// only persistent actors can be seen, and where they are placed by the game while unloaded is what op=check reports.
//
// THE PROMPT AND THE PRESS. Virtual functions on the door form: GetActivateText for the words, Activate for the press, so the
// game's own activate control and its own routing decide the key on keyboard and controller. No key is read here. The press
// shows a choice: Knock, Pick the lock, Leave. A lock is only ever picked because the player chose it, so a refusal can never
// fall through to the lockpicking path, and another mod's automatic picking (with its crime) cannot follow a second press.
//
// SIMPLE ACTIVATE. po3's Simple Activate rewrites the HUD text after this hook: with Hide Text on doors it drops the first line
// and colours the name line of a crime door. Its source is public (powerof3/SimpleActivateSKSE). While it is loaded the prompt
// is one line, "Knock: name", which it neither shortens nor colours; otherwise two lines, as the game writes them.
//
// WHO DECIDES (the owner, 2026-11-06: "some kind of dialogue where they have to agree to let me in"). A Knock no longer
// opens the door by itself. The resident with the best relationship to the player answers (adults before children, the
// awake before the asleep) and decides (Judge): a friend or better always lets you in; anybody else when the player's
// Speech reaches iKnockSpeech plus penalties for night, for having been woken, for a bounty in their hold, for being seen
// stealing there, and for each rank of dislike. Refused, the box offers Persuade (Speech against iPersuadeSpeech plus the
// same penalties) and Intimidate (the player's level raised by Speech, against theirs scaled by their confidence through
// the game's fIntimidateConfidenceMult settings), once each per refusal, besides Pick the lock and Leave.
//
// THE VOICE, WITHOUT A PLUGIN (the owner, 06:25: "the no esp option"). The answer is spoken from the door with the native
// ObjectReference.Say(topic, speakAs) through the Papyrus VM: the door speaks, the resident's voice type and conditions
// choose the line. The two topics are vanilla DialogueGeneric topics, found by script in Skyrim.esm (2026-11-06) and set
// in [Voice]: let in 039C2A (Trespass Against NPC: "I'll let you stay. This time.", "If you were anyone else, I'd make
// you leave."), refused 06AEA2 (Time To Go: "You need to leave.", with a line for every voice type). A voice type that
// topic has no line for says nothing; the on-screen message still says what happened.
//
// PERSUADE AND INTIMIDATE LINES (the owner, 2026-11-07: "different voice lines for the intimidate or persuade options").
// Found by script in Skyrim.esm (DIAL/INFO records, voice files counted in Skyrim - Voices_en0.bsa), 2026-11-07. Vanilla has
// no stand-alone generic persuade or intimidate topic: every speech check is a quest topic, and the generic answers they
// share are infos of DialogueGenericSharedInfo 01F319 (no conditions, recorded for the 42 generic voice types of
// Skyrim.esm, children included), reached through DialogueFavorGeneric branch topics guarded by GetIntimidateSuccess /
// GetBribeSuccess. So each outcome here names the infos, and the shared topic is narrowed to the chosen one while it is
// said (Narrow). One is picked in turn per outcome:
//   persuaded       0E0CBA "Okay. Just this once." (not MaleOldGrumpy, MaleYoungEager), 0E0CBB "How can I argue with
//                   that?" (not MaleOldGrumpy), 0E0CBC "Okay, okay. You've talked me into it." (not MaleOldGrumpy,
//                   MaleYoungEager)
//   not persuaded   0E0CC3 "You expect me to believe that?", 0E0CC4 "Nah. I don't think so.", 0E0CC5 "That doesn't change
//                   anything."
//   intimidated     0E0CBD "No need to get rough!", 0E0CBE "Whatever you say!", 0E0CBF "Don't hurt me! You win."
//   not intimidated 0E0CC8 "I don't have to take that from you!"; and 03FF6A "I don't have to take this from the likes of
//                   you." (the one info of DialogueFavorGeneric topic 060C94, no conditions, 42 voice types). The other
//                   two vanilla failure lines (0E0CC6, 0E0CC7) promise a fist fight, which never follows here, so are left out.
// A voice type outside those 42 (a DLC or mod voice), or one a line was not recorded for, gets the let-in or refuse topic.
//
// LETTING IN (BeginLetIn). The door's lock is opened in memory and the destination cell's Public flag set in memory, with no
// change flag on either form, so the game's save carries nothing of it. One let-in at a time, kept in one co-save record so a
// save made inside the house still works when loaded. It ends the next time a load finishes with the player in an exterior
// cell: the door locks again and the cell goes back to what it was. Knock, Persuade and Intimidate all let in this way.
//
// NO REPORT TO THE GUARDS (the owner, 2026-11-07). Being let in is not a crime by any path: the lock is opened for the
// player, so no lockpicking crime; walking through an owned door is no crime; a trespass is the only crime of being in a
// home, and the game does not count it in a Public cell - including when a resident's sleep package locks the doors at
// night, which is what makes a home "closed". As a second guard, InitiateTresPassPackage (the warnings that end in a
// trespass alarm, used by residents and guards alike) is refused on every Character and on the player while the player is
// in the let-in cell (SuppressTrespass), with a log line. What stays a crime stays one: stealing, pickpocketing, assault.
#include "PCH.h"

#include "Knock.h"

#include "Settings.h"

#include "utils/Logger.h"
#include "utils/Strings.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <deque>
#include <optional>
#include <thread>
#include <tuple>
#include <format>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <windows.h>
#undef GetObject   // windows.h maps it to GetObjectW, hiding BSScript::Variable::GetObject

namespace knock
{
	namespace
	{
		constexpr std::uint32_t kCoSaveRecord = 'LETI';
		constexpr std::uint32_t kCoSaveVersion = 1;
		constexpr RE::FormID kPlayerFaction = 0x000DB1;   // Skyrim.esm, PlayerFaction
		constexpr RE::FormID kManakinRace = 0x10760A;     // Skyrim.esm, ManakinRace: a mannequin is not a resident

		// Chosen from the census: every keyword here marks a place that is fought through, none marks a lived-in door of the 276.
		constexpr std::array kDungeonKeywords = {
			"LocTypeDungeon", "LocTypeAnimalDen", "LocTypeBanditCamp", "LocTypeDraugrCrypt", "LocTypeFalmerHive",
			"LocTypeWarlockLair", "LocTypeVampireLair", "LocTypeForswornCamp", "LocTypeGiantCamp", "LocTypeHagravenNest",
			"LocTypeDragonPriestLair", "LocTypeDwarvenAutomatons", "LocTypeSprigganGrove", "LocTypeNordicRuin", "LocTypeMine"
		};

		enum class Verdict
		{
			kOff, kNoPlayer, kSneaking, kInCombat, kNotLocked, kPlayerHasKey, kNoTeleport, kNotFromExterior, kNotToInterior,
			kExcluded, kDungeon, kPlayersOwn, kNotAHome, kHostile, kHome
		};

		const char* Name(Verdict a_v)
		{
			switch (a_v)
			{
			case Verdict::kOff: return "off";
			case Verdict::kNoPlayer: return "no player";
			case Verdict::kSneaking: return "sneaking";
			case Verdict::kInCombat: return "in combat";
			case Verdict::kNotLocked: return "not locked";
			case Verdict::kPlayerHasKey: return "player has the key";
			case Verdict::kNoTeleport: return "no teleport";
			case Verdict::kNotFromExterior: return "door not in an exterior";
			case Verdict::kNotToInterior: return "destination not an interior";
			case Verdict::kExcluded: return "on the exclusion list";
			case Verdict::kDungeon: return "dungeon keyword";
			case Verdict::kPlayersOwn: return "the player's own";
			case Verdict::kNotAHome: return "no owner, no resident";
			case Verdict::kHostile: return "hostile";
			case Verdict::kHome: return "home";
			}
			return "?";
		}

		struct Home
		{
			RE::TESObjectCELL* cell{ nullptr };
			RE::TESForm* owner{ nullptr };
			RE::TESNPC* ownerNPC{ nullptr };
			RE::TESFaction* ownerFaction{ nullptr };
			std::vector<RE::Actor*> residents;   // who may answer, living and enabled (see FindResidents)
			const char* method{ "none" };       // how they were found
		};

		enum class Act : std::uint8_t { kKnock, kPersuade, kIntimidate, kPick, kLeave, kPeek };

		// What the decision weighs, read from the game (or set through DevBench, see Overrides).
		struct Inputs
		{
			RE::Actor* speaker{ nullptr };
			int speech{ 0 };            // the player's Speech skill
			int playerLevel{ 1 };
			int theirLevel{ 1 };
			int rank{ 0 };              // relationship rank to the player: 4 lover .. 0 acquaintance .. -4 archnemesis
			int confidence{ 2 };        // their Confidence: 0 cowardly .. 4 foolhardy
			float confidenceMult{ 1.0F };
			bool night{ false };
			bool asleep{ false };
			bool child{ false };
			std::uint32_t bounty{ 0 };  // the player's crime gold with their crime faction
			std::uint32_t stolen{ 0 };  // goods seen stolen from that faction, by value
			RE::FormID crimeFaction{ 0 };
		};

		struct Verdicts
		{
			int penalty{ 0 };
			int knockNeed{ 0 };
			int persuadeNeed{ 0 };
			float yourIntimidation{ 0.0F };
			float theirIntimidation{ 0.0F };
			bool friendly{ false };
			bool knock{ false };
			bool persuade{ false };
			bool intimidate{ false };
		};

		// DevBench op=set: values that replace what the game says, until op=clear or the next game load.
		struct Overrides
		{
			std::optional<int> speech, rank, playerLevel, theirLevel, confidence, bounty, stolen;
			std::optional<bool> night, asleep;
		};

		// The last refusal: Persuade and Intimidate are offered for this door until both are used or iRetryHours pass.
		struct Refusal
		{
			RE::FormID door{ 0 };
			RE::FormID speaker{ 0 };
			float at{ 0.0F };           // game days
			bool triedPersuade{ false };
			bool triedIntimidate{ false };
		};

		struct LetIn
		{
			RE::FormID door{ 0 };
			RE::FormID cell{ 0 };
			bool cellWasPublic{ false };
		};

		// The actor table: built once per game, from the forms in memory.
		std::vector<RE::FormID> g_actors;
		bool g_tableBuilt = false;
		std::uint32_t g_tableActors = 0;
		long long g_tableMs = 0;

		std::unordered_set<RE::FormID> g_excludedIDs;
		std::unordered_set<std::string> g_excludedEditorIDs;
		std::uint32_t g_excludedUnresolved = 0;

		RE::FormID g_promptDoor = 0;    // the door last shown as Knock
		RE::FormID g_choiceDoor = 0;    // the door the choice box is open for
		std::vector<Act> g_buttons;     // what each button of the open box does
		RE::FormID g_passDoor = 0;      // the next Activate on this door goes to the game untouched
		bool g_simpleActivate = false;
		LetIn g_letIn;
		Refusal g_refusal;
		Overrides g_over;
		RE::TESTopic* g_letInTopic = nullptr;
		RE::TESTopic* g_refuseTopic = nullptr;
		std::string g_lastDecision = "null";   // JSON of the last Judge that acted

		std::uint32_t g_knocks = 0, g_letIns = 0, g_nobody = 0, g_refused = 0, g_promptsShown = 0;
		std::uint32_t g_persuaded = 0, g_notPersuaded = 0, g_intimidated = 0, g_notIntimidated = 0, g_says = 0, g_sayFailed = 0;
		std::uint32_t g_trespassBlocked = 0, g_trespassBlockedThisLetIn = 0;
		std::deque<std::string> g_events;

		// THE SPEECH-CHECK LINES (see the file header). The game's generic speech-check answers are shared infos of
		// DialogueGenericSharedInfo, so each line is a topic and the one info of it to speak.
		enum class Speech : std::uint8_t { kPersuaded, kNotPersuaded, kIntimidated, kNotIntimidated };
		struct SpeechLine
		{
			RE::FormID topic;
			RE::FormID info;
			std::array<RE::FormID, 2> lacks;   // generic voice types with no recording of this line
		};
		constexpr RE::FormID kSharedInfoTopic = 0x01F319;   // DialogueGenericSharedInfo
		constexpr RE::FormID kMaleOldGrumpy = 0x013AD7, kMaleYoungEager = 0x013AD1;
		constexpr std::array<std::array<SpeechLine, 3>, 4> kSpeechLines = { {
			{ { { kSharedInfoTopic, 0x0E0CBA, { kMaleOldGrumpy, kMaleYoungEager } }, { kSharedInfoTopic, 0x0E0CBB, { kMaleOldGrumpy, 0 } }, { kSharedInfoTopic, 0x0E0CBC, { kMaleOldGrumpy, kMaleYoungEager } } } },
			{ { { kSharedInfoTopic, 0x0E0CC3, {} }, { kSharedInfoTopic, 0x0E0CC4, {} }, { kSharedInfoTopic, 0x0E0CC5, {} } } },
			{ { { kSharedInfoTopic, 0x0E0CBD, {} }, { kSharedInfoTopic, 0x0E0CBE, {} }, { kSharedInfoTopic, 0x0E0CBF, {} } } },
			{ { { kSharedInfoTopic, 0x0E0CC8, {} }, { 0x060C94, 0x03FF6A, {} }, { 0, 0, {} } } },
		} };
		// The 42 voice types of Skyrim.esm every line above was recorded for (less each line's `lacks`). Any other voice type,
		// from a DLC or a mod, gets the let-in or refuse topic instead.
		constexpr std::array<RE::FormID, 42> kGenericVoices = {
			0x013AD1, 0x013AD2, 0x013AD3, 0x013AD4, 0x013AD5, 0x013AD6, 0x013AD7, 0x013AD8, 0x013AD9, 0x013ADA, 0x013ADB, 0x013ADC,
			0x013ADD, 0x013ADE, 0x013AE0, 0x013AE1, 0x013AE2, 0x013AE3, 0x013AE4, 0x013AE5, 0x013AE6, 0x013AE7, 0x013AE8, 0x013AE9,
			0x013AEA, 0x013AEB, 0x013AEC, 0x013AED, 0x013AEE, 0x013AEF, 0x013AF0, 0x013AF1, 0x013AF2, 0x013AF3, 0x013BC3, 0x01B55F,
			0x09843B, 0x09844F, 0x0AA8D3, 0x0E5003, 0x0EA266, 0x0EA267
		};
		struct ResolvedLine
		{
			RE::TESTopic* topic{ nullptr };
			RE::TESTopicInfo* info{ nullptr };
			const SpeechLine* line{ nullptr };
		};
		std::array<std::vector<ResolvedLine>, 4> g_speech;

		// While a shared info is being spoken, its topic holds only that info (Say picks from the topic's list); the list
		// is put back a few seconds later, once the game has picked the line, or at the next line or game load.
		struct Narrowed
		{
			RE::TESTopic* topic{ nullptr };
			RE::TESTopicInfo** infos{ nullptr };
			std::uint32_t count{ 0 };
		};
		Narrowed g_narrowed;
		RE::TESTopicInfo* g_oneInfo[1]{ nullptr };
		std::uint32_t g_narrowGen = 0;

		std::string Lower(std::string a_s)
		{
			for (char& c : a_s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
			return a_s;
		}

		std::string Escape(std::string_view a_in)
		{
			std::string out;
			for (const char c : a_in)
			{
				if (c == '\\') { out += "\\\\"; }
				else if (c == '"') { out += "\\\""; }
				else if (c == '\n') { out += "\\n"; }
				else { out += c; }
			}
			return out;
		}

		void Notify(const char* a_text)
		{
#if RUNTIME_LINE == 17
			RE::SendHUDMessage::ShowHUDMessage(a_text, nullptr, true);
#else
			RE::DebugNotification(a_text);
#endif
		}

		void Event(std::string a_text)
		{
			logger::info("{}", a_text);
			g_events.push_back(std::move(a_text));
			if (g_events.size() > 8) { g_events.pop_front(); }
		}

		void Dirty()
		{
			if (auto* player = RE::PlayerCharacter::GetSingleton()) { player->GetPlayerRuntimeData().playerFlags.shouldUpdateCrosshair = true; }
		}

		RE::FormID PlayerFactionID()
		{
			static RE::FormID id = 0;
			if (!id)
			{
				if (auto* dh = RE::TESDataHandler::GetSingleton()) { id = dh->LookupFormID(kPlayerFaction, "Skyrim.esm"); }
			}
			return id;
		}

		// Every actor in memory. Rebuilt for a knock, so actors of a cell still loaded count.
		void BuildTable()
		{
			if (g_tableBuilt) { return; }
			g_tableBuilt = true;
			g_actors.clear();
			g_tableActors = 0;
			const auto start = std::chrono::steady_clock::now();

			const auto& [map, lock] = RE::TESForm::GetAllForms();
			if (!map)
			{
				logger::warn("the game's form table is not available; nobody can answer");
				return;
			}
			RE::BSReadLockGuard guard{ lock };
			for (auto& [id, form] : *map)
			{
				if (!form || form->GetFormType() != RE::FormType::ActorCharacter) { continue; }
				auto* actor = form->As<RE::Actor>();
				if (!actor || actor->IsPlayerRef()) { continue; }
				g_actors.push_back(id);
			}
			g_tableActors = static_cast<std::uint32_t>(g_actors.size());
			g_tableMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
			logger::debug("actor table: {} actors in memory, {} ms", g_tableActors, g_tableMs);
		}

		// Where the game holds this actor now.
		RE::TESObjectCELL* CurrentCell(const RE::Actor* a_actor)
		{
			auto* cell = a_actor->GetParentCell();
			return cell ? cell : a_actor->GetSaveParentCell();
		}

		bool IsMannequin(RE::Actor* a_actor)
		{
			auto* race = a_actor ? a_actor->GetRace() : nullptr;
			return race && race->GetLocalFormID() == kManakinRace;
		}

		RE::FIGHT_REACTION ReactionToPlayer(RE::TESFaction* a_faction)
		{
			if (!a_faction) { return RE::FIGHT_REACTION::kNeutral; }
			const auto playerFaction = PlayerFactionID();
			for (const auto* reaction : a_faction->reactions)
			{
				if (reaction && reaction->form && reaction->form->GetFormID() == playerFaction) { return reaction->fightReaction; }
			}
			return RE::FIGHT_REACTION::kNeutral;
		}

		bool Matches(const RE::TESForm* a_form)
		{
			if (!a_form) { return false; }
			if (g_excludedIDs.contains(a_form->GetFormID())) { return true; }
			if (g_excludedEditorIDs.empty()) { return false; }
			const char* editorID = a_form->GetFormEditorID();
			return editorID && editorID[0] && g_excludedEditorIDs.contains(Lower(editorID));
		}

		bool DungeonKeyword(const RE::BGSLocation* a_location)
		{
			int guard = 0;
			for (auto* location = a_location; location && guard < 16; location = location->parentLoc, ++guard)
			{
				for (const char* keyword : kDungeonKeywords)
				{
					if (location->HasKeywordString(keyword)) { return true; }
				}
			}
			return false;
		}

		bool ExcludedLocation(const RE::BGSLocation* a_location)
		{
			int guard = 0;
			for (auto* location = a_location; location && guard < 16; location = location->parentLoc, ++guard)
			{
				if (Matches(location)) { return true; }
			}
			return false;
		}

		bool Eligible(RE::Actor* a_actor) { return a_actor && !a_actor->IsDisabled() && !a_actor->IsDead() && !IsMannequin(a_actor); }

		// The cell the actor was placed in by its plugin: where it lives, wherever the game holds it now.
		RE::TESObjectCELL* EditorCell(RE::Actor* a_actor)
		{
			RE::NiPoint3 pos, rot;
			RE::TESForm* worldOrCell = nullptr;
			if (!a_actor || !a_actor->GetEditorLocation(pos, rot, worldOrCell, nullptr) || !worldOrCell) { return nullptr; }
			return worldOrCell->As<RE::TESObjectCELL>();
		}

		// WHO MAY ANSWER. Only somebody inside the house now (the owner, 2026-11-05 and 2026-11-07: "you dont get a response if
		// no one is home, you just get a message saying that no one's home, come back later"). 1.0.0 widened the search to
		// residents placed there and to the owner or owning faction wherever the game held them, so a resident out in town
		// answered and let the player in (Nexus report, 2026-11-07); that widening is gone. Every actor in memory is in the table,
		// and the search takes, in order, stopping at the first that finds anybody:
		//   1. in the cell now (the actor's current parent cell, else its save cell);
		//   2. lives there (editor cell) and the game holds them in no cell at all, so nothing says they are out.
		// Alive, enabled, not a mannequin; hostility is judged on whoever is found. A resident who is not persistent and
		// whose cell is unloaded is not in memory at all, so cannot be found: that door says nobody is home.
		void FindResidents(Home& a_home)
		{
			BuildTable();
			const auto pass = [&a_home](const char* a_method, auto&& a_match) {
				for (const auto id : g_actors)
				{
					auto* actor = RE::TESForm::LookupByID<RE::Actor>(id);
					if (Eligible(actor) && a_match(actor)) { a_home.residents.push_back(actor); }
				}
				if (!a_home.residents.empty()) { a_home.method = a_method; }
				return !a_home.residents.empty();
			};
			if (pass("in the cell now", [&a_home](RE::Actor* a) { return CurrentCell(a) == a_home.cell; })) { return; }
			pass("lives there, held in no cell", [&a_home](RE::Actor* a) { return !CurrentCell(a) && EditorCell(a) == a_home.cell; });
		}

		bool LivesThere(RE::TESObjectCELL* a_cell)
		{
			for (const auto id : g_actors)
			{
				auto* actor = RE::TESForm::LookupByID<RE::Actor>(id);
				if (Eligible(actor) && EditorCell(actor) == a_cell) { return true; }
			}
			return false;
		}

		Verdict Classify(RE::TESObjectREFR* a_door, Home& a_home)
		{
			if (!settings::enabled) { return Verdict::kOff; }
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || !a_door) { return Verdict::kNoPlayer; }
			if (player->IsSneaking()) { return Verdict::kSneaking; }
			if (player->IsInCombat()) { return Verdict::kInCombat; }

			auto* lock = a_door->GetLock();
			if (!lock || !a_door->IsLocked()) { return Verdict::kNotLocked; }
			if (lock->key && player->GetInventoryCounts()[lock->key] > 0) { return Verdict::kPlayerHasKey; }

			const auto destHandle = a_door->extraList.GetTeleportLinkedDoor();
			const auto destPtr = destHandle.get();
			auto* dest = destPtr.get();
			if (!dest) { return Verdict::kNoTeleport; }
			auto* here = a_door->GetParentCell();
			if (!here || here->IsInteriorCell()) { return Verdict::kNotFromExterior; }
			auto* cell = dest->GetParentCell() ? dest->GetParentCell() : dest->GetSaveParentCell();
			if (!cell || !cell->IsInteriorCell()) { return Verdict::kNotToInterior; }
			a_home.cell = cell;

			auto* location = cell->GetLocation();
			if (Matches(a_door) || Matches(a_door->GetBaseObject()) || Matches(dest) || Matches(cell) || ExcludedLocation(location)) { return Verdict::kExcluded; }
			if (DungeonKeyword(location)) { return Verdict::kDungeon; }

			auto* owner = a_door->extraList.GetOwner();
			if (!owner) { owner = dest->extraList.GetOwner(); }
			if (!owner) { owner = cell->GetOwner(); }
			a_home.owner = owner;
			if (owner)
			{
				a_home.ownerNPC = owner->As<RE::TESNPC>();
				a_home.ownerFaction = owner->As<RE::TESFaction>();
				if (a_home.ownerNPC && a_home.ownerNPC == player->GetActorBase()) { return Verdict::kPlayersOwn; }
				if (a_home.ownerFaction && player->IsInFaction(a_home.ownerFaction)) { return Verdict::kPlayersOwn; }
			}

			FindResidents(a_home);

			// Somebody's home even while they are out (the owner, 2026-11-07: "You should still be able to knock and have the
			// option to lockpick or peek"): an owner, or a living actor placed in that cell. Out, the box still opens; a knock
			// gets the no-one's-home message, and no conversation choice is offered.
			if (!owner && a_home.residents.empty() && !LivesThere(a_home.cell)) { return Verdict::kNotAHome; }

			if (ReactionToPlayer(a_home.ownerFaction) == RE::FIGHT_REACTION::kEnemy) { return Verdict::kHostile; }
			for (auto* actor : a_home.residents)
			{
				if (actor->IsHostileToActor(player)) { return Verdict::kHostile; }
			}
			if (a_home.ownerNPC)
			{
				for (const auto& rank : a_home.ownerNPC->factions)
				{
					if (ReactionToPlayer(rank.faction) == RE::FIGHT_REACTION::kEnemy) { return Verdict::kHostile; }
				}
			}
			return Verdict::kHome;
		}

		bool IsDay()
		{
			auto* calendar = RE::Calendar::GetSingleton();
			if (!calendar) { return true; }
			const float hour = calendar->GetHour();
			const float start = static_cast<float>(settings::dayStartHour);
			const float end = static_cast<float>(settings::dayEndHour);
			return start <= end ? (hour >= start && hour < end) : (hour >= start || hour < end);
		}

		// Relationship rank to the player, as the game holds it: the levels run Lover (0) .. Friend (3), Acquaintance (4) ..
		// Archnemesis (8), so rank = 4 - level (4 lover, 1 friend, 0 acquaintance, -4 archnemesis). No record: 0.
		int RankOf(RE::TESNPC* a_npc)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* playerBase = player ? player->GetActorBase() : nullptr;
			if (!a_npc || !playerBase) { return 0; }
			const auto* relationship = RE::BGSRelationship::GetRelationship(a_npc, playerBase);
			if (!relationship) { return 0; }
			return 4 - static_cast<int>(relationship->level.underlying());
		}

		bool IsFriend(RE::TESNPC* a_npc) { return RankOf(a_npc) >= 1; }

		bool IsAsleep(RE::Actor* a_actor)
		{
			if (!a_actor) { return false; }
			if (auto* state = a_actor->AsActorState())
			{
				const auto sleep = state->GetSitSleepState();
				if (sleep == RE::SIT_SLEEP_STATE::kWantToSleep || sleep == RE::SIT_SLEEP_STATE::kWaitingForSleepAnim || sleep == RE::SIT_SLEEP_STATE::kIsSleeping) { return true; }
			}
			const auto* package = a_actor->GetCurrentPackage();
			return package && static_cast<std::uint32_t>(package->packData.packType.get()) == static_cast<std::uint32_t>(RE::PACKAGE_PROCEDURE_TYPE::kSleep);
		}

		// Who answers: the best relationship first, then adults before children, then the awake before the asleep.
		RE::Actor* ChooseSpeaker(const Home& a_home)
		{
			RE::Actor* best = nullptr;
			std::tuple<int, int, int> bestKey{ -100, 0, 0 };
			for (auto* actor : a_home.residents)
			{
				if (!actor) { continue; }
				const std::tuple<int, int, int> key{ RankOf(actor->GetActorBase()), actor->IsChild() ? 0 : 1, IsAsleep(actor) ? 0 : 1 };
				if (!best || key > bestKey)
				{
					best = actor;
					bestKey = key;
				}
			}
			return best;
		}

		float GameSetting(const char* a_name, float a_fallback)
		{
			auto* collection = RE::GameSettingCollection::GetSingleton();
			auto* setting = collection ? collection->GetSetting(a_name) : nullptr;
			if (!setting)
			{
				logger::warn("game setting {} not found; using {}", a_name, a_fallback);
				return a_fallback;
			}
			return setting->GetFloat();
		}

		// The vanilla intimidation idea: the target's confidence scales how hard they are to scare.
		float ConfidenceMult(int a_confidence)
		{
			switch (std::clamp(a_confidence, 0, 4))
			{
			case 0: return GameSetting("fIntimidateConfidenceMultCowardly", 1.0F);
			case 1: return GameSetting("fIntimidateConfidenceMultCautious", 1.0F);
			case 3: return GameSetting("fIntimidateConfidenceMultBrave", 1.0F);
			case 4: return GameSetting("fIntimidateConfidenceMultFoolhardy", 1.0F);
			default: return GameSetting("fIntimidateConfidenceMultAverage", 1.0F);
			}
		}

		Inputs Gather(const Home& a_home)
		{
			Inputs in;
			auto* player = RE::PlayerCharacter::GetSingleton();
			in.speaker = ChooseSpeaker(a_home);
			if (!player || !in.speaker) { return in; }

			auto* playerAV = player->AsActorValueOwner();
			auto* theirAV = in.speaker->AsActorValueOwner();
			in.speech = playerAV ? static_cast<int>(playerAV->GetActorValue(RE::ActorValue::kSpeech)) : 0;
			in.playerLevel = player->GetLevel();
			in.theirLevel = in.speaker->GetLevel();
			in.confidence = theirAV ? static_cast<int>(theirAV->GetActorValue(RE::ActorValue::kConfidence)) : 2;
			in.rank = RankOf(in.speaker->GetActorBase());
			in.night = !IsDay();
			in.asleep = IsAsleep(in.speaker);
			in.child = in.speaker->IsChild();

			if (const auto* faction = in.speaker->GetCrimeFaction())
			{
				in.crimeFaction = faction->GetFormID();
				in.bounty = player->GetCrimeGoldValue(faction);
				const auto& stolen = player->GetCrimeValue().stolenItemValueMap;
				if (const auto it = stolen.find(faction); it != stolen.end()) { in.stolen = static_cast<std::uint32_t>(std::max(0, it->second.witnessed)); }
			}

			if (g_over.speech) { in.speech = *g_over.speech; }
			if (g_over.rank) { in.rank = *g_over.rank; }
			if (g_over.playerLevel) { in.playerLevel = *g_over.playerLevel; }
			if (g_over.theirLevel) { in.theirLevel = *g_over.theirLevel; }
			if (g_over.confidence) { in.confidence = *g_over.confidence; }
			if (g_over.bounty) { in.bounty = static_cast<std::uint32_t>(std::max(0, *g_over.bounty)); }
			if (g_over.stolen) { in.stolen = static_cast<std::uint32_t>(std::max(0, *g_over.stolen)); }
			if (g_over.night) { in.night = *g_over.night; }
			if (g_over.asleep) { in.asleep = *g_over.asleep; }
			in.confidenceMult = ConfidenceMult(in.confidence);
			return in;
		}

		Verdicts Judge(const Inputs& a_in)
		{
			Verdicts v;
			v.penalty = (a_in.night ? settings::nightPenalty : 0) + (a_in.asleep ? settings::asleepPenalty : 0) +
			            (settings::bountyGold > 0 && a_in.bounty >= static_cast<std::uint32_t>(settings::bountyGold) ? settings::bountyPenalty : 0) +
			            (settings::thiefGold > 0 && a_in.stolen >= static_cast<std::uint32_t>(settings::thiefGold) ? settings::thiefPenalty : 0) +
			            std::max(0, -a_in.rank) * settings::dislikePenalty;
			v.knockNeed = settings::knockSpeech + v.penalty;
			v.persuadeNeed = settings::persuadeSpeech + v.penalty;
			v.friendly = a_in.rank >= settings::friendRank;
			v.knock = v.friendly || a_in.speech >= v.knockNeed;
			v.persuade = a_in.speech >= v.persuadeNeed;
			v.yourIntimidation = static_cast<float>(a_in.playerLevel) * (1.0F + static_cast<float>(a_in.speech) / 100.0F);
			v.theirIntimidation = static_cast<float>(a_in.theirLevel) * a_in.confidenceMult;
			v.intimidate = v.yourIntimidation >= v.theirIntimidation + static_cast<float>(settings::intimidateMargin);
			return v;
		}

		std::string DecisionJson(const char* a_what, const Inputs& a_in, const Verdicts& a_v)
		{
			return std::format(
				"{{\"what\":\"{}\",\"speaker\":\"{:08X}\",\"name\":\"{}\",\"inputs\":{{\"speech\":{},\"playerLevel\":{},\"theirLevel\":{},\"rank\":{},\"confidence\":{},\"confidenceMult\":{:.2f},"
				"\"night\":{},\"asleep\":{},\"child\":{},\"bounty\":{},\"stolen\":{},\"crimeFaction\":\"{:08X}\"}},"
				"\"result\":{{\"penalty\":{},\"knockNeed\":{},\"persuadeNeed\":{},\"yourIntimidation\":{:.1f},\"theirIntimidation\":{:.1f},\"friendly\":{},\"knock\":{},\"persuade\":{},\"intimidate\":{}}}}}",
				a_what, a_in.speaker ? a_in.speaker->GetFormID() : 0, a_in.speaker ? Escape(a_in.speaker->GetDisplayFullName()) : "",
				a_in.speech, a_in.playerLevel, a_in.theirLevel, a_in.rank, a_in.confidence, a_in.confidenceMult,
				a_in.night, a_in.asleep, a_in.child, a_in.bounty, a_in.stolen, a_in.crimeFaction,
				a_v.penalty, a_v.knockNeed, a_v.persuadeNeed, a_v.yourIntimidation, a_v.theirIntimidation, a_v.friendly, a_v.knock, a_v.persuade, a_v.intimidate);
		}

		std::string WithName(const char* a_text, RE::Actor* a_actor)
		{
			std::string out = a_text ? a_text : "";
			const char* name = a_actor ? a_actor->GetDisplayFullName() : nullptr;
			const std::string who = name && name[0] ? name : "?";
			for (auto at = out.find("{name}"); at != std::string::npos; at = out.find("{name}", at + who.size())) { out.replace(at, 6, who); }
			return out;
		}

		// Papyrus calls back here when Say has run; the result is None either way, so this only logs.
		class SayDone : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			explicit SayDone(std::string a_what) : what(std::move(a_what)) {}
			void operator()(RE::BSScript::Variable) override { logger::debug("Say ({}) ran", what); }
			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}

		private:
			std::string what;
		};

		// Whether the topic has a line whose conditions pass for this speaker (voice type and the rest), as the game will ask.
		// Read from the forms in memory, so it holds for a speaker whose cell is not loaded.
		bool HasLineFor(RE::TESTopic* a_topic, RE::Actor* a_speaker)
		{
			if (!a_topic || !a_speaker || !a_topic->topicInfos) { return false; }
			auto* player = RE::PlayerCharacter::GetSingleton();
			for (std::uint32_t i = 0; i < a_topic->numTopicInfos; ++i)
			{
				auto* info = a_topic->topicInfos[i];
				if (info && info->objConditions.IsTrue(a_speaker, player)) { return true; }
			}
			return false;
		}

		// The native ObjectReference.Say(topic, speakAs, false) on the door: the line is played at the door, spoken as the
		// resident, in their voice. Packing the door binds it a base ObjectReference object, as the VM does for any form it
		// is handed.
		bool SayAs(RE::TESObjectREFR* a_door, RE::TESTopic* a_topic, RE::Actor* a_speaker, const char* a_what)
		{
			if (!a_door || !a_topic || !a_speaker)
			{
				++g_sayFailed;
				logger::warn("Say ({}) skipped: door {}, topic {}, speaker {}", a_what, a_door != nullptr, a_topic != nullptr, a_speaker != nullptr);
				return false;
			}
			const bool loaded = a_speaker->Is3DLoaded();
			if (!HasLineFor(a_topic, a_speaker))
			{
				++g_sayFailed;
				auto* base = a_speaker->GetActorBase();
				const auto* voice = base ? base->GetVoiceType() : nullptr;
				logger::info("Say ({}) not sent: topic {:08X} has no line for {:08X} (voice type {:08X}); the message only", a_what, a_topic->GetFormID(), a_speaker->GetFormID(), voice ? voice->GetFormID() : 0);
				return false;
			}
			auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			if (!vm)
			{
				++g_sayFailed;
				logger::warn("Say ({}) skipped: the Papyrus VM is not ready", a_what);
				return false;
			}
			RE::BSScript::Variable self;
			self.Pack(a_door);
			auto object = self.GetObject();
			if (!object)
			{
				++g_sayFailed;
				logger::warn("Say ({}) skipped: door {:08X} has no script object", a_what, a_door->GetFormID());
				return false;
			}
			auto* args = RE::MakeFunctionArguments(static_cast<RE::TESTopic*>(a_topic), static_cast<RE::Actor*>(a_speaker), false);
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback{ new SayDone(a_what) };
			const bool sent = vm->DispatchMethodCall(object, RE::BSFixedString("Say"), args, callback);
			if (sent) { ++g_says; }
			else { ++g_sayFailed; }
			logger::info("Say ({}): topic {:08X}{} at door {:08X} as {:08X} ({}) {}", a_what, a_topic->GetFormID(),
				a_topic->numTopicInfos == 1 && a_topic->topicInfos[0] ? std::format(" info {:08X}", a_topic->topicInfos[0]->GetFormID()) : std::string(),
				a_door->GetFormID(), a_speaker->GetFormID(), loaded ? "speaker loaded" : "speaker not loaded", sent ? "sent" : "REFUSED by the VM");
			return sent;
		}

		void RestoreNarrowed()
		{
			if (!g_narrowed.topic) { return; }
			g_narrowed.topic->topicInfos = g_narrowed.infos;
			g_narrowed.topic->numTopicInfos = g_narrowed.count;
			g_narrowed = {};
		}

		void Narrow(RE::TESTopic* a_topic, RE::TESTopicInfo* a_info)
		{
			RestoreNarrowed();
			if (a_topic->numTopicInfos == 1 && a_topic->topicInfos && a_topic->topicInfos[0] == a_info) { return; }
			g_narrowed = { a_topic, a_topic->topicInfos, a_topic->numTopicInfos };
			g_oneInfo[0] = a_info;
			a_topic->topicInfos = g_oneInfo;
			a_topic->numTopicInfos = 1;
			const auto gen = ++g_narrowGen;
			std::thread([gen]() {
				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
				if (auto* tasks = SKSE::GetTaskInterface())
				{
					tasks->AddTask([gen]() {
						if (gen == g_narrowGen) { RestoreNarrowed(); }
					});
				}
			}).detach();
		}

		// A persuade or intimidate outcome: one of its lines in the speaker's voice, or the fallback topic when their voice
		// type has none of them.
		bool SaySpeech(RE::TESObjectREFR* a_door, Speech a_speech, RE::Actor* a_speaker, RE::TESTopic* a_fallback, const char* a_what)
		{
			auto* base = a_speaker ? a_speaker->GetActorBase() : nullptr;
			const auto* voice = base ? base->GetVoiceType() : nullptr;
			const RE::FormID vt = voice ? voice->GetFormID() : 0;
			std::vector<const ResolvedLine*> usable;
			if (vt && std::find(kGenericVoices.begin(), kGenericVoices.end(), vt) != kGenericVoices.end())
			{
				for (const auto& line : g_speech[static_cast<std::size_t>(a_speech)])
				{
					if (std::find(line.line->lacks.begin(), line.line->lacks.end(), vt) == line.line->lacks.end()) { usable.push_back(&line); }
				}
			}
			if (usable.empty())
			{
				logger::info("Say ({}): no speech-check line for voice type {:08X}; the {} topic instead", a_what, vt, a_fallback == g_letInTopic ? "let-in" : "refuse");
				return SayAs(a_door, a_fallback, a_speaker, a_what);
			}
			static std::uint32_t turn = static_cast<std::uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
			const auto* pick = usable[(turn++) % usable.size()];
			Narrow(pick->topic, pick->info);
			return SayAs(a_door, pick->topic, a_speaker, a_what);
		}

		// TRESPASS (the owner, 2026-11-07: "if you're let in by knocking on the door or persuading, then they don't report
		// you to the guards"). The Public flag on the let-in cell is what keeps the household from treating the player as an
		// intruder. As a second guard, while a let-in holds and the player stands in its cell, nobody may start the trespass
		// package on the player (the warnings that end in a trespass crime): the game's Character::InitiateTresPassPackage is
		// refused here. Knock, Persuade and Intimidate all let in through BeginLetIn, so all three are covered alike.
		bool SuppressTrespass(RE::Actor* a_who)
		{
			if (!g_letIn.cell) { return false; }
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* cell = player ? player->GetParentCell() : nullptr;
			if (!cell || cell->GetFormID() != g_letIn.cell) { return false; }
			++g_trespassBlocked;
			if (++g_trespassBlockedThisLetIn == 1)
			{
				Event(std::format("trespass suppressed: {:08X} would have started the trespass package in let-in cell {:08X}", a_who ? a_who->GetFormID() : 0, g_letIn.cell));
			}
			else
			{
				logger::debug("trespass suppressed again ({}): {:08X} in cell {:08X}", g_trespassBlockedThisLetIn, a_who ? a_who->GetFormID() : 0, g_letIn.cell);
			}
			return true;
		}

		struct TrespassHooks
		{
			static bool Character(RE::Character* a_this, RE::TrespassPackage* a_package)
			{
				return SuppressTrespass(a_this) ? false : _Character(a_this, a_package);
			}
			static bool Player(RE::PlayerCharacter* a_this, RE::TrespassPackage* a_package)
			{
				return SuppressTrespass(a_this) ? false : _Player(a_this, a_package);
			}
			static inline REL::Relocation<decltype(Character)> _Character;
			static inline REL::Relocation<decltype(Player)> _Player;
		};

		void ApplyLetIn()
		{
			auto* door = RE::TESForm::LookupByID<RE::TESObjectREFR>(g_letIn.door);
			auto* cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(g_letIn.cell);
			if (auto* lock = door ? door->GetLock() : nullptr) { lock->SetLocked(false); }
			if (cell) { cell->cellFlags.set(RE::TESObjectCELL::Flag::kPublicArea); }
		}

		// LEAVING FROM INSIDE (Nexus report, 2026-11-07: let in while the owner was out, the player had to pick the lock to get
		// out). This mod never locks the inside door: EndLetIn locks only the outside door, and only once a load has finished
		// with the player outside. The inside door is locked by the game itself - a resident's AI package locks the doors of
		// their home as they leave or turn in - and 161 vanilla homes carry a lock record on the inside door. So while a let-in
		// holds, any locked door of the let-in house that leads outside is unlocked in memory when the player looks at it or
		// presses it: whoever let you in, you can always walk out.
		bool UnlockWayOut(RE::TESObjectREFR* a_door)
		{
			if (!g_letIn.door || !a_door || !a_door->IsLocked()) { return false; }
			auto* here = a_door->GetParentCell();
			if (!here || here->GetFormID() != g_letIn.cell) { return false; }
			const auto destPtr = a_door->extraList.GetTeleportLinkedDoor().get();
			auto* dest = destPtr.get();
			auto* outside = dest ? (dest->GetParentCell() ? dest->GetParentCell() : dest->GetSaveParentCell()) : nullptr;
			if (!outside || outside->IsInteriorCell()) { return false; }
			auto* lock = a_door->GetLock();
			if (!lock) { return false; }
			lock->SetLocked(false);
			Event(std::format("way out: door {:08X} inside the let-in cell {:08X} was locked by the game; unlocked", a_door->GetFormID(), g_letIn.cell));
			return true;
		}

		void EndLetIn(const char* a_why)
		{
			if (!g_letIn.door) { return; }
			auto* door = RE::TESForm::LookupByID<RE::TESObjectREFR>(g_letIn.door);
			auto* cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(g_letIn.cell);
			if (auto* lock = door ? door->GetLock() : nullptr) { lock->SetLocked(true); }
			if (cell && !g_letIn.cellWasPublic) { cell->cellFlags.reset(RE::TESObjectCELL::Flag::kPublicArea); }
			Event(std::format("let-in over ({}): door {:08X} locked again, cell {:08X} back to {}", a_why, g_letIn.door, g_letIn.cell, g_letIn.cellWasPublic ? "public" : "private"));
			g_letIn = {};
		}

		void BeginLetIn(RE::TESObjectREFR* a_door, const Home& a_home)
		{
			EndLetIn("another door");
			g_letIn.door = a_door->GetFormID();
			g_letIn.cell = a_home.cell->GetFormID();
			g_letIn.cellWasPublic = a_home.cell->cellFlags.any(RE::TESObjectCELL::Flag::kPublicArea);
			g_trespassBlockedThisLetIn = 0;
			ApplyLetIn();
		}

		float Now()
		{
			auto* calendar = RE::Calendar::GetSingleton();
			return calendar ? calendar->GetCurrentGameTime() : 0.0F;
		}

		bool RefusalActive(RE::FormID a_door)
		{
			return g_refusal.door && g_refusal.door == a_door && (Now() - g_refusal.at) * 24.0F < static_cast<float>(settings::retryHours);
		}

		// The refusal box (Persuade, Intimidate) only while the one who refused is still inside: nobody home, no conversation.
		bool RefuserHome(RE::FormID a_door, const Home& a_home)
		{
			if (!RefusalActive(a_door)) { return false; }
			auto* speaker = RE::TESForm::LookupByID<RE::Actor>(g_refusal.speaker);
			return speaker && std::find(a_home.residents.begin(), a_home.residents.end(), speaker) != a_home.residents.end();
		}

		bool CanPick(RE::TESObjectREFR* a_door) { return a_door && a_door->GetLockLevel() != RE::LOCK_LEVEL::kRequiresKey; }

		const char* Label(Act a_act)
		{
			switch (a_act)
			{
			case Act::kKnock: return strings::TR("DSK_Knock", "Knock");
			case Act::kPersuade: return strings::TR("DSK_ChoicePersuade", "Persuade");
			case Act::kIntimidate: return strings::TR("DSK_ChoiceIntimidate", "Intimidate");
			case Act::kPick: return strings::TR("DSK_ChoicePick", "Pick the lock");
			case Act::kLeave: return strings::TR("DSK_ChoiceLeave", "Leave");
			case Act::kPeek: return strings::TR("DSK_ChoicePeek", "Peek through the keyhole");
			}
			return "?";
		}

		const char* ActName(Act a_act)
		{
			switch (a_act)
			{
			case Act::kKnock: return "knock";
			case Act::kPersuade: return "persuade";
			case Act::kIntimidate: return "intimidate";
			case Act::kPick: return "pick";
			case Act::kLeave: return "leave";
			case Act::kPeek: return "peek";
			}
			return "?";
		}

		void OnChoice(std::uint8_t a_button);

		// PEEK (the owner, 2026-11-06). With Take a Peek (Nexus 66908) loaded, the boxes offer its peek: the same function its
		// hold-Sneak and ability paths call, Keyhole_Exit.TakingAPeek() on its quest's player alias. It reads the crosshair for
		// the door and refuses a drawn weapon or a fight itself, with its own messages. Without it the choice is not shown.
		constexpr const char* kPeekPlugin = "Take A Peek - New Stealth Mechanic.esp";
		constexpr RE::FormID kPeekQuest = 0x802;  // Keyhole_Quest

		RE::TESQuest* PeekQuest()
		{
			static bool looked = false;
			static RE::TESQuest* quest = nullptr;
			if (!looked)
			{
				auto* data = RE::TESDataHandler::GetSingleton();
				if (!data) { return nullptr; }
				looked = true;
				quest = data->LookupForm<RE::TESQuest>(kPeekQuest, kPeekPlugin);
				logger::info("Take a Peek {}", quest ? "loaded: the knock boxes offer Peek" : "not loaded: no Peek choice");
			}
			return quest;
		}

		bool Peek()
		{
			auto* quest = PeekQuest();
			auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			auto* policy = vm ? vm->GetObjectHandlePolicy() : nullptr;
			if (!quest || !policy) { return false; }
			for (auto* base : quest->aliases)
			{
				auto* alias = base ? skyrim_cast<RE::BGSRefAlias*>(base) : nullptr;
				if (!alias) { continue; }
				const RE::VMHandle handle = policy->GetHandleForObject(RE::BGSRefAlias::VMTYPEID, alias);
				if (handle == 0) { continue; }
				RE::BSTSmartPointer<RE::BSScript::Object> object;
				if (!vm->FindBoundObject(handle, "Keyhole_Exit", object) || !object) { continue; }
				auto args = RE::MakeFunctionArguments();
				RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
				return vm->DispatchMethodCall(handle, RE::BSFixedString("Keyhole_Exit"), RE::BSFixedString("TakingAPeek"), args, callback);
			}
			logger::warn("Take a Peek: no alias carries Keyhole_Exit; the peek was not sent");
			return false;
		}

		bool ShowBox(RE::TESObjectREFR* a_door, const std::string& a_body, std::vector<Act> a_acts)
		{
			// Signature from CommonLibSSE-NG 7.2's MessageBoxMenu::Create, the same function on every runtime: it reads
			// the buttons up to a nullptr and hands the callback the button's index plus the offset.
			using func_t = bool(const char*, void (*)(std::uint8_t), std::uint8_t, std::int32_t, std::int32_t, ...);
			static REL::Relocation<func_t> create{ RELOCATION_ID(51420, 52269) };

			std::array<const char*, 4> labels{ nullptr, nullptr, nullptr, nullptr };
			for (std::size_t i = 0; i < a_acts.size() && i < labels.size(); ++i) { labels[i] = Label(a_acts[i]); }
			if (a_acts.size() > labels.size()) { a_acts.resize(labels.size()); }
			g_choiceDoor = a_door->GetFormID();
			g_buttons = std::move(a_acts);
			const bool shown = create(a_body.c_str(), OnChoice, std::uint8_t{ 0 }, 0, 10, labels[0], labels[1], labels[2], labels[3], nullptr);
			if (!shown)
			{
				g_choiceDoor = 0;
				g_buttons.clear();
				logger::warn("the choice box for {:08X} did not open", a_door->GetFormID());
			}
			return shown;
		}

		std::string Place(RE::TESObjectREFR* a_door, const Home& a_home)
		{
			const char* place = a_home.cell ? a_home.cell->GetFullName() : nullptr;
			if (!place || !place[0]) { place = a_door->GetDisplayFullName(); }
			return place ? place : "";
		}

		// The first box: Knock, Pick the lock (not for a key-only lock), Leave.
		bool ShowChoice(RE::TESObjectREFR* a_door, const Home& a_home)
		{
			std::vector<Act> acts{ Act::kKnock };
			if (CanPick(a_door)) { acts.push_back(Act::kPick); }
			if (PeekQuest()) { acts.push_back(Act::kPeek); }
			acts.push_back(Act::kLeave);
			return ShowBox(a_door, std::format("{}\n{}", Place(a_door, a_home), strings::TR("DSK_ChoiceBody", "The door is locked.")), std::move(acts));
		}

		// After a refusal: what is left of Persuade and Intimidate, then Pick the lock and Leave.
		bool ShowRefusalBox(RE::TESObjectREFR* a_door, const Home& a_home)
		{
			std::vector<Act> acts;
			if (!g_refusal.triedPersuade) { acts.push_back(Act::kPersuade); }
			if (!g_refusal.triedIntimidate) { acts.push_back(Act::kIntimidate); }
			if (CanPick(a_door)) { acts.push_back(Act::kPick); }
			if (acts.size() < 3 && PeekQuest()) { acts.push_back(Act::kPeek); }  // the box holds four buttons, Leave included
			acts.push_back(Act::kLeave);
			auto* speaker = RE::TESForm::LookupByID<RE::Actor>(g_refusal.speaker);
			return ShowBox(a_door, std::format("{}\n{}", Place(a_door, a_home), WithName(strings::TR("DSK_RefusedBody", "{name} won't open the door."), speaker)), std::move(acts));
		}

		// A refusal's box comes up by itself once the spoken line has had time to play (the box would pause it). The next
		// press on the door shows it too, so walking away loses nothing.
		void ScheduleRefusalBox(RE::FormID a_door)
		{
			if (g_refusal.triedPersuade && g_refusal.triedIntimidate) { return; }
			std::thread([a_door]() {
				std::this_thread::sleep_for(std::chrono::milliseconds(2500));
				auto* tasks = SKSE::GetTaskInterface();
				if (!tasks) { return; }
				tasks->AddTask([a_door]() {
					if (!RefusalActive(a_door) || g_choiceDoor) { return; }
					auto* ui = RE::UI::GetSingleton();
					if (!ui || ui->GameIsPaused() || ui->IsMenuOpen(RE::MessageBoxMenu::MENU_NAME) || ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME)) { return; }
					auto* player = RE::PlayerCharacter::GetSingleton();
					auto* door = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_door);
					if (!player || !door || player->GetParentCell() != door->GetParentCell() || player->GetPosition().GetDistance(door->GetPosition()) > 400.0F) { return; }
					Home home;
					if (Classify(door, home) == Verdict::kHome && RefuserHome(a_door, home)) { ShowRefusalBox(door, home); }
				});
			}).detach();
		}

		void LetInNow(RE::TESObjectREFR* a_door, const Home& a_home, const Inputs& a_in, const char* a_key, const char* a_english, const char* a_how,
			std::optional<Speech> a_speech = std::nullopt)
		{
			++g_letIns;
			BeginLetIn(a_door, a_home);
			g_refusal = {};
			if (a_speech) { SaySpeech(a_door, *a_speech, a_in.speaker, g_letInTopic, a_how); }
			else { SayAs(a_door, g_letInTopic, a_in.speaker, a_how); }
			Notify(WithName(strings::TR(a_key, a_english), a_in.speaker).c_str());
			Event(std::format("{} on {:08X}: let in by {:08X} (speech {}, rank {}, {}{})", a_how, a_door->GetFormID(), a_in.speaker->GetFormID(), a_in.speech, a_in.rank,
				a_in.night ? "night" : "day", a_in.asleep ? ", woken" : ""));
			Dirty();   // the door's own prompt now: it is unlocked, and the next press opens it
		}

		void RefuseNow(RE::TESObjectREFR* a_door, const Inputs& a_in, const char* a_key, const char* a_english, const char* a_how,
			std::optional<Speech> a_speech = std::nullopt)
		{
			if (a_speech) { SaySpeech(a_door, *a_speech, a_in.speaker, g_refuseTopic, a_how); }
			else { SayAs(a_door, g_refuseTopic, a_in.speaker, a_how); }
			Notify(WithName(strings::TR(a_key, a_english), a_in.speaker).c_str());
			Event(std::format("{} on {:08X}: refused by {:08X} (speech {}, rank {}, {}{}, bounty {}, stolen {})", a_how, a_door->GetFormID(), a_in.speaker->GetFormID(), a_in.speech, a_in.rank,
				a_in.night ? "night" : "day", a_in.asleep ? ", woken" : "", a_in.bounty, a_in.stolen));
			ScheduleRefusalBox(a_door->GetFormID());
		}

		// Knock, Persuade or Intimidate on a door the rule has called a home. Nothing here opens the door by itself: a let-in
		// unlocks it and the player's next press goes through it.
		void Attempt(RE::TESObjectREFR* a_door, const Home& a_home, Act a_act)
		{
			const auto in = Gather(a_home);
			if (!in.speaker)
			{
				++g_nobody;
				Notify(strings::TR("DSK_NoAnswer", "No one's home. Come back later."));
				Event(std::format("{} on {:08X}: nobody home in cell {:08X} (no living resident in it now): no answer, no voice", ActName(a_act), a_door->GetFormID(), a_home.cell->GetFormID()));
				return;
			}
			const auto v = Judge(in);
			g_lastDecision = DecisionJson(ActName(a_act), in, v);
			logger::info("decision ({} found by: {}, {} candidate(s)): {}", ActName(a_act), a_home.method, a_home.residents.size(), g_lastDecision);

			if (a_act == Act::kKnock)
			{
				++g_knocks;
				if (v.knock)
				{
					if (in.asleep) { LetInNow(a_door, a_home, in, "DSK_MsgWokeLetIn", "You woke {name}, who unlocks the door.", v.friendly ? "knock, friend" : "knock"); }
					else { LetInNow(a_door, a_home, in, "DSK_MsgLetIn", "{name} unlocks the door.", v.friendly ? "knock, friend" : "knock"); }
					return;
				}
				++g_refused;
				if (!RefusalActive(a_door->GetFormID())) { g_refusal = {}; }
				g_refusal.door = a_door->GetFormID();
				g_refusal.speaker = in.speaker->GetFormID();
				g_refusal.at = Now();
				if (in.asleep) { RefuseNow(a_door, in, "DSK_MsgWokeRefused", "You woke {name}, who won't let you in.", "knock"); }
				else { RefuseNow(a_door, in, "DSK_MsgRefused", "{name} won't let you in.", "knock"); }
				return;
			}

			const bool persuade = a_act == Act::kPersuade;
			if (persuade) { g_refusal.triedPersuade = true; }
			else { g_refusal.triedIntimidate = true; }
			g_refusal.door = a_door->GetFormID();
			g_refusal.speaker = in.speaker->GetFormID();
			if (persuade ? v.persuade : v.intimidate)
			{
				if (persuade) { ++g_persuaded; LetInNow(a_door, a_home, in, "DSK_MsgPersuaded", "{name} is persuaded and unlocks the door.", "persuade", Speech::kPersuaded); }
				else { ++g_intimidated; LetInNow(a_door, a_home, in, "DSK_MsgIntimidated", "{name} backs down and unlocks the door.", "intimidate", Speech::kIntimidated); }
				return;
			}
			if (persuade) { ++g_notPersuaded; RefuseNow(a_door, in, "DSK_MsgNotPersuaded", "{name} is not persuaded.", "persuade", Speech::kNotPersuaded); }
			else { ++g_notIntimidated; RefuseNow(a_door, in, "DSK_MsgNotIntimidated", "{name} is not afraid of you.", "intimidate", Speech::kNotIntimidated); }
		}

		// Runs the game's own Activate on the door, past our hook: the game's lock path runs.
		void PassToGame(RE::TESObjectREFR* a_door)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!a_door || !player) { return; }
			g_passDoor = a_door->GetFormID();
			a_door->ActivateRef(player, 0, nullptr, 1, false);
			g_passDoor = 0;
		}

		void Run(RE::FormID a_id, Act a_act)
		{
			auto* door = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_id);
			if (!door) { return; }
			if (a_act == Act::kLeave) { return; }
			if (a_act == Act::kPeek)
			{
				Event(std::format("player chose to peek through {:08X}: {}", a_id, Peek() ? "sent to Take a Peek" : "not sent"));
				return;
			}
			if (a_act == Act::kPick)
			{
				Event(std::format("player chose to pick {:08X}", a_id));
				PassToGame(door);
				return;
			}
			g_tableBuilt = false;   // who is in memory at this moment
			Home home;
			const auto verdict = Classify(door, home);
			if (verdict != Verdict::kHome)
			{
				Event(std::format("{} on {:08X} dropped: {}", ActName(a_act), a_id, Name(verdict)));
				return;
			}
			Attempt(door, home, a_act);
		}

		void OnChoice(std::uint8_t a_button)
		{
			const RE::FormID id = g_choiceDoor;
			const auto buttons = g_buttons;
			g_choiceDoor = 0;
			g_buttons.clear();
			auto* tasks = SKSE::GetTaskInterface();
			if (!id || !tasks || a_button >= buttons.size()) { return; }
			const Act act = buttons[a_button];
			// After the box has closed, on the main thread.
			tasks->AddTask([id, act]() { Run(id, act); });
		}

		struct DoorHooks
		{
			// CommonLibSSE-NG names this argument a_activator. It must be the door's reference for this to do anything, so it
			// is checked against this door's own form, and anything else takes the game's own text.
			static bool GetActivateText(RE::TESObjectDOOR* a_this, RE::TESObjectREFR* a_ref, RE::BSString& a_dst)
			{
				if (a_ref && a_ref->GetBaseObject() == a_this)
				{
					UnlockWayOut(a_ref);
					const auto id = a_ref->GetFormID();
					Home home;
					const auto verdict = Classify(a_ref, home);
					logger::debug("prompt on {:08X}: {}", id, Name(verdict));
					if (verdict == Verdict::kHome)
					{
						g_promptDoor = id;
						++g_promptsShown;
						const char* name = home.cell ? home.cell->GetFullName() : nullptr;
						if (!name || !name[0]) { name = a_ref->GetDisplayFullName(); }
						const char* word = strings::TR("DSK_Knock", "Knock");
						a_dst = (g_simpleActivate ? std::format("{}: {}", word, name ? name : "") : std::format("{}\n{}", word, name ? name : "")).c_str();
						return true;
					}
					if (g_promptDoor == id) { g_promptDoor = 0; }
				}
				return _GetActivateText(a_this, a_ref, a_dst);
			}

			static bool Activate(RE::TESObjectDOOR* a_this, RE::TESObjectREFR* a_target, RE::TESObjectREFR* a_activator, std::uint8_t a_arg3, RE::TESBoundObject* a_object, std::int32_t a_count)
			{
				if (a_activator && a_activator->IsPlayerRef()) { UnlockWayOut(a_target); }
				if (a_target && a_activator && a_activator->IsPlayerRef() && a_target->GetFormID() != g_passDoor && a_target->GetFormID() == g_promptDoor)
				{
					Home home;
					if (Classify(a_target, home) == Verdict::kHome &&
						(RefuserHome(a_target->GetFormID(), home) ? ShowRefusalBox(a_target, home) : ShowChoice(a_target, home)))
					{
						return true;
					}
				}
				return _Activate(a_this, a_target, a_activator, a_arg3, a_object, a_count);
			}

			static inline REL::Relocation<decltype(GetActivateText)> _GetActivateText;
			static inline REL::Relocation<decltype(Activate)> _Activate;
		};

		struct Sinks :
			public RE::BSTEventSink<RE::MenuOpenCloseEvent>
		{
			static Sinks* Get()
			{
				static Sinks sinks;
				return &sinks;
			}

			RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
			{
				if (a_event && !a_event->opening && g_letIn.door && a_event->menuName == RE::LoadingMenu::MENU_NAME)
				{
					auto* player = RE::PlayerCharacter::GetSingleton();
					auto* cell = player ? player->GetParentCell() : nullptr;
					if (cell && !cell->IsInteriorCell()) { EndLetIn("the player is outside"); }
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};
	}

	void Install()
	{
		REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE_TESObjectDOOR[0] };
		DoorHooks::_Activate = vtable.write_vfunc(0x37, DoorHooks::Activate);
		DoorHooks::_GetActivateText = vtable.write_vfunc(0x4C, DoorHooks::GetActivateText);
		logger::info("door hooks: Activate (slot 0x37) was at {:X}, GetActivateText (slot 0x4C) was at {:X}", DoorHooks::_Activate.address(), DoorHooks::_GetActivateText.address());

		REL::Relocation<std::uintptr_t> character{ RE::VTABLE_Character[0] };
		TrespassHooks::_Character = character.write_vfunc(0xD7, TrespassHooks::Character);
		REL::Relocation<std::uintptr_t> player{ RE::VTABLE_PlayerCharacter[0] };
		TrespassHooks::_Player = player.write_vfunc(0xD7, TrespassHooks::Player);
		logger::info("trespass hooks: InitiateTresPassPackage (slot 0xD7) on Character was at {:X}, on PlayerCharacter at {:X}", TrespassHooks::_Character.address(), TrespassHooks::_Player.address());

		if (auto* ui = RE::UI::GetSingleton())
		{
			ui->AddEventSink<RE::MenuOpenCloseEvent>(Sinks::Get());
		}
		else
		{
			logger::error("the UI singleton is not ready; a let-in will only end at the next knock");
		}

		g_simpleActivate = GetModuleHandleW(L"po3_SimpleActivateSKSE") != nullptr;
		logger::info("Simple Activate {}: the prompt is {}", g_simpleActivate ? "is loaded" : "is not loaded", g_simpleActivate ? "one line" : "two lines");
	}

	void ResolveExclusions()
	{
		g_excludedIDs.clear();
		g_excludedEditorIDs.clear();
		g_excludedUnresolved = 0;
		auto* dh = RE::TESDataHandler::GetSingleton();
		for (const auto& entry : settings::exclusions)
		{
			if (const auto bar = entry.find('|'); bar != std::string::npos)
			{
				RE::FormID id = 0;
				try { id = static_cast<RE::FormID>(std::stoul(entry.substr(bar + 1), nullptr, 16)); }
				catch (...) { logger::warn("exclusion \"{}\": the form ID is not a number", entry); }
				const auto full = (dh && id) ? dh->LookupFormID(id & 0x00FFFFFF, entry.substr(0, bar)) : 0;
				if (full) { g_excludedIDs.insert(full); }
				else { ++g_excludedUnresolved; logger::debug("exclusion \"{}\": not in this load order", entry); }
			}
			else
			{
				g_excludedEditorIDs.insert(Lower(entry));
				if (auto* form = RE::TESForm::LookupByEditorID(entry)) { g_excludedIDs.insert(form->GetFormID()); }
			}
		}
		logger::info("exclusions: {} forms, {} editor IDs, {} not in this load order", g_excludedIDs.size(), g_excludedEditorIDs.size(), g_excludedUnresolved);

		// The two voiced topics, "Plugin|0xID".
		const auto topic = [dh](const std::string& a_entry, const char* a_what) -> RE::TESTopic* {
			const auto bar = a_entry.find('|');
			if (!dh || bar == std::string::npos)
			{
				logger::warn("[Voice] {} \"{}\" is not Plugin|0xID; that line is not spoken", a_what, a_entry);
				return nullptr;
			}
			RE::FormID id = 0;
			try { id = static_cast<RE::FormID>(std::stoul(a_entry.substr(bar + 1), nullptr, 16)); }
			catch (...)
			{
				logger::warn("[Voice] {} \"{}\": the form ID is not a number", a_what, a_entry);
				return nullptr;
			}
			auto* form = dh->LookupForm<RE::TESTopic>(id & 0x00FFFFFF, a_entry.substr(0, bar));
			if (!form) { logger::warn("[Voice] {} \"{}\" is not a dialogue topic in this load order; that line is not spoken", a_what, a_entry); }
			else { logger::info("[Voice] {}: topic {:08X}", a_what, form->GetFormID()); }
			return form;
		};
		g_letInTopic = topic(settings::letInTopic, "let in");
		g_refuseTopic = topic(settings::refuseTopic, "refuse");

		// The speech-check lines, Skyrim.esm only.
		RestoreNarrowed();
		for (std::size_t s = 0; s < kSpeechLines.size(); ++s)
		{
			g_speech[s].clear();
			for (const auto& line : kSpeechLines[s])
			{
				if (!line.topic) { continue; }
				auto* t = dh ? dh->LookupForm<RE::TESTopic>(line.topic, "Skyrim.esm") : nullptr;
				auto* i = dh ? dh->LookupForm<RE::TESTopicInfo>(line.info, "Skyrim.esm") : nullptr;
				bool inTopic = false;
				for (std::uint32_t n = 0; t && t->topicInfos && n < t->numTopicInfos; ++n) { inTopic = inTopic || t->topicInfos[n] == i; }
				if (t && i && inTopic) { g_speech[s].push_back({ t, i, &line }); }
				else { logger::warn("[Voice] speech-check line {:06X}/{:06X} is not in this Skyrim.esm; left out", line.topic, line.info); }
			}
		}
		logger::info("[Voice] speech-check lines: persuaded {}, not persuaded {}, intimidated {}, not intimidated {}", g_speech[0].size(), g_speech[1].size(), g_speech[2].size(), g_speech[3].size());
	}

	void OnGameLoaded()
	{
		RestoreNarrowed();
		g_tableBuilt = false;
		g_promptDoor = 0;
		g_choiceDoor = 0;
		g_buttons.clear();
		g_passDoor = 0;
		g_refusal = {};
		g_over = {};
	}

	void RefreshPrompt()
	{
		g_promptDoor = 0;
		Dirty();
	}

	void OnSave(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc || !g_letIn.door) { return; }
		if (a_intfc->OpenRecord(kCoSaveRecord, kCoSaveVersion))
		{
			a_intfc->WriteRecordData(g_letIn);
		}
	}

	void OnLoad(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) { return; }
		std::uint32_t type, version, length;
		while (a_intfc->GetNextRecordInfo(type, version, length))
		{
			if (type != kCoSaveRecord || version != kCoSaveVersion || length != sizeof(LetIn)) { continue; }
			LetIn saved;
			if (a_intfc->ReadRecordData(saved) == sizeof(LetIn) &&
				a_intfc->ResolveFormID(saved.door, saved.door) && a_intfc->ResolveFormID(saved.cell, saved.cell))
			{
				g_letIn = saved;
				ApplyLetIn();
				logger::info("the save holds a let-in: door {:08X}, cell {:08X}", g_letIn.door, g_letIn.cell);
			}
		}
	}

	void OnRevert(SKSE::SerializationInterface*)
	{
		EndLetIn("another game is loading");
		OnGameLoaded();
	}

	std::string StateJson()
	{
		auto* calendar = RE::Calendar::GetSingleton();
		std::string events;
		for (const auto& e : g_events) { events += (events.empty() ? "\"" : ",\"") + Escape(e) + "\""; }
		const auto opt = [](const auto& a_v) { return a_v ? std::to_string(static_cast<int>(*a_v)) : std::string("null"); };
		const std::string overrides = std::format("{{\"speech\":{},\"rank\":{},\"playerLevel\":{},\"theirLevel\":{},\"confidence\":{},\"bounty\":{},\"stolen\":{},\"night\":{},\"asleep\":{}}}",
			opt(g_over.speech), opt(g_over.rank), opt(g_over.playerLevel), opt(g_over.theirLevel), opt(g_over.confidence), opt(g_over.bounty), opt(g_over.stolen), opt(g_over.night), opt(g_over.asleep));
		const std::string decision = std::format("{{\"friendRank\":{},\"knockSpeech\":{},\"persuadeSpeech\":{},\"nightPenalty\":{},\"asleepPenalty\":{},\"bountyGold\":{},\"bountyPenalty\":{},"
			"\"thiefGold\":{},\"thiefPenalty\":{},\"dislikePenalty\":{},\"intimidateMargin\":{},\"retryHours\":{}}}",
			settings::friendRank, settings::knockSpeech, settings::persuadeSpeech, settings::nightPenalty, settings::asleepPenalty, settings::bountyGold, settings::bountyPenalty,
			settings::thiefGold, settings::thiefPenalty, settings::dislikePenalty, settings::intimidateMargin, settings::retryHours);
		return std::format(
			"{{\"ok\":true,\"enabled\":{},\"dayStart\":{},\"dayEnd\":{},\"hour\":{:.2f},\"isDay\":{},\"simpleActivate\":{},\"promptDoor\":\"{:08X}\",\"choiceDoor\":\"{:08X}\",\"buttons\":{},"
			"\"letIn\":{{\"door\":\"{:08X}\",\"cell\":\"{:08X}\",\"cellWasPublic\":{}}},"
			"\"refusal\":{{\"door\":\"{:08X}\",\"speaker\":\"{:08X}\",\"active\":{},\"triedPersuade\":{},\"triedIntimidate\":{}}},"
			"\"topics\":{{\"letIn\":\"{:08X}\",\"refuse\":\"{:08X}\"}},\"settings\":{},\"overrides\":{},\"lastDecision\":{},"
			"\"table\":{{\"built\":{},\"actors\":{},\"ms\":{}}},"
			"\"exclusions\":{{\"forms\":{},\"editorIDs\":{},\"unresolved\":{}}},"
			"\"counts\":{{\"prompts\":{},\"knocks\":{},\"letIn\":{},\"nobody\":{},\"refused\":{},\"persuaded\":{},\"notPersuaded\":{},\"intimidated\":{},\"notIntimidated\":{},\"says\":{},\"sayFailed\":{},\"trespassBlocked\":{}}},\"events\":[{}]}}",
			settings::enabled, settings::dayStartHour, settings::dayEndHour, calendar ? calendar->GetHour() : -1.0F, IsDay(), g_simpleActivate, g_promptDoor, g_choiceDoor, g_buttons.size(),
			g_letIn.door, g_letIn.cell, g_letIn.cellWasPublic,
			g_refusal.door, g_refusal.speaker, RefusalActive(g_refusal.door), g_refusal.triedPersuade, g_refusal.triedIntimidate,
			g_letInTopic ? g_letInTopic->GetFormID() : 0, g_refuseTopic ? g_refuseTopic->GetFormID() : 0, decision, overrides, g_lastDecision,
			g_tableBuilt, g_tableActors, g_tableMs,
			g_excludedIDs.size(), g_excludedEditorIDs.size(), g_excludedUnresolved,
			g_promptsShown, g_knocks, g_letIns, g_nobody, g_refused, g_persuaded, g_notPersuaded, g_intimidated, g_notIntimidated, g_says, g_sayFailed, g_trespassBlocked, events);
	}

	std::string CheckJson(std::uint32_t a_refID)
	{
		auto* door = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_refID);
		if (!door) { return std::format("{{\"ok\":false,\"error\":\"no reference {:08X}\"}}", a_refID); }
		g_tableBuilt = false;
		Home home;
		const auto verdict = Classify(door, home);
		std::string residents;
		for (auto* actor : home.residents)
		{
			residents += std::format("{}{{\"id\":\"{:08X}\",\"name\":\"{}\",\"rank\":{},\"child\":{},\"asleep\":{},\"loaded\":{},\"lineLetIn\":{},\"lineRefuse\":{}}}", residents.empty() ? "" : ",", actor->GetFormID(), Escape(actor->GetDisplayFullName()),
				RankOf(actor->GetActorBase()), actor->IsChild(), IsAsleep(actor), actor->Is3DLoaded(), HasLineFor(g_letInTopic, actor), HasLineFor(g_refuseTopic, actor));
		}
		// Who the owner's and the cell's people are by name, and where the game holds them: the proof of what an unloaded cell shows.
		std::string people;
		for (const auto id : g_actors)
		{
			auto* actor = RE::TESForm::LookupByID<RE::Actor>(id);
			if (!actor || !home.cell) { continue; }
			const bool ownersNPC = home.ownerNPC && actor->GetActorBase() == home.ownerNPC;
			const bool member = home.ownerFaction && actor->IsInFaction(home.ownerFaction);
			if (!ownersNPC && !member) { continue; }
			const auto* parent = actor->GetParentCell();
			const auto* save = actor->GetSaveParentCell();
			people += std::format("{}{{\"id\":\"{:08X}\",\"name\":\"{}\",\"parent\":\"{:08X}\",\"save\":\"{:08X}\",\"process\":{},\"dead\":{},\"disabled\":{}}}", people.empty() ? "" : ",",
				id, Escape(actor->GetDisplayFullName()), parent ? parent->GetFormID() : 0, save ? save->GetFormID() : 0, actor->GetActorRuntimeData().currentProcess != nullptr, actor->IsDead(), actor->IsDisabled());
		}
		std::string decision = "null";
		if (verdict == Verdict::kHome)
		{
			const auto in = Gather(home);
			if (in.speaker) { decision = DecisionJson("check", in, Judge(in)); }
		}
		return std::format(
			"{{\"ok\":true,\"verdict\":\"{}\",\"foundBy\":\"{}\",\"decision\":{},\"refusalActive\":{},\"cell\":\"{:08X}\",\"cellAttached\":{},\"owner\":\"{:08X}\",\"ownerType\":\"{}\",\"residents\":[{}],\"ownerPeople\":[{}]}}",
			Name(verdict), home.method, decision, RefusalActive(a_refID), home.cell ? home.cell->GetFormID() : 0, home.cell ? home.cell->IsAttached() : false, home.owner ? home.owner->GetFormID() : 0,
			home.ownerNPC ? "npc" : (home.ownerFaction ? "faction" : (home.owner ? "other" : "none")), residents, people);
	}

	std::string ActJson(std::uint32_t a_refID, const char* a_act)
	{
		auto* door = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_refID);
		if (!door) { return std::format("{{\"ok\":false,\"error\":\"no reference {:08X}\"}}", a_refID); }
		const std::string_view act = a_act ? a_act : "";
		const Act which = act == "persuade" ? Act::kPersuade : (act == "intimidate" ? Act::kIntimidate : Act::kKnock);
		g_tableBuilt = false;
		Home home;
		const auto verdict = Classify(door, home);
		if (verdict != Verdict::kHome) { return std::format("{{\"ok\":false,\"verdict\":\"{}\"}}", Name(verdict)); }
		Attempt(door, home, which);
		return std::format("{{\"ok\":true,\"op\":\"{}\",\"decision\":{},\"state\":{}}}", ActName(which), g_lastDecision, StateJson());
	}

	std::string ChoiceJson(std::uint32_t a_refID, bool a_refusalBox)
	{
		auto* door = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_refID);
		if (!door) { return std::format("{{\"ok\":false,\"error\":\"no reference {:08X}\"}}", a_refID); }
		Home home;
		const auto verdict = Classify(door, home);
		if (verdict != Verdict::kHome) { return std::format("{{\"ok\":false,\"verdict\":\"{}\"}}", Name(verdict)); }
		const bool refusal = a_refusalBox || RefusalActive(a_refID);
		if (refusal && !g_refusal.door) { return R"({"ok":false,"error":"no refusal to answer; knock first"})"; }
		const bool shown = refusal ? ShowRefusalBox(door, home) : ShowChoice(door, home);
		return std::format("{{\"ok\":{},\"box\":\"{}\",\"buttons\":{}}}", shown, refusal ? "refusal" : "knock", g_buttons.size());
	}

	std::string SetJson(const std::function<std::optional<int>(const char*)>& a_field, bool a_clear)
	{
		if (a_clear) { g_over = {}; }
		else
		{
			if (auto v = a_field("speech")) { g_over.speech = v; }
			if (auto v = a_field("rank")) { g_over.rank = v; }
			if (auto v = a_field("playerLevel")) { g_over.playerLevel = v; }
			if (auto v = a_field("theirLevel")) { g_over.theirLevel = v; }
			if (auto v = a_field("confidence")) { g_over.confidence = v; }
			if (auto v = a_field("bounty")) { g_over.bounty = v; }
			if (auto v = a_field("stolen")) { g_over.stolen = v; }
			if (auto v = a_field("night")) { g_over.night = *v != 0; }
			if (auto v = a_field("asleep")) { g_over.asleep = *v != 0; }
			if (auto v = a_field("resetRefusal"); v && *v) { g_refusal = {}; }
		}
		logger::info("DevBench {} the decision overrides", a_clear ? "cleared" : "set");
		return StateJson();
	}

	std::string EndLetInJson()
	{
		EndLetIn("DevBench");
		return R"({"ok":true,"op":"endletin"})";
	}
}
