// Dynamic Simple Knock System SKSE - the rule and the two door hooks.
//
// THE RULE (Classify). A locked door, standing in an exterior cell, that teleports to an interior, is somebody's home when the
// door, its other half or the destination cell has an owner, or a living resident is placed in that cell. Left alone: the
// player sneaking or in combat, the player holding the key, the player's own home, the [Exclusions] list, a destination
// location with a dungeon keyword, anybody there hostile to the player. Interior-to-interior doors are left alone on purpose:
// the census shows they are mostly jarls' quarters, temple sanctums and vaults, and nobody knocks from inside the building.
//
// WHO ANSWERS (the owner, 2026-11-05). Only somebody in the destination cell right now: a living, enabled actor whose current
// cell is that cell. Nobody there: "Nobody answers." The actors are read from the forms in memory, so while the cell is unloaded
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
// LETTING IN (BeginLetIn). The door's lock is opened in memory and the destination cell's Public flag set in memory, with no
// change flag on either form, so the game's save carries nothing of it. One let-in at a time, kept in one co-save record so a
// save made inside the house still works when loaded. It ends the next time a load finishes with the player in an exterior
// cell: the door locks again and the cell goes back to what it was.
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
#include <format>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <windows.h>

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
			std::vector<RE::Actor*> residents;   // in the cell now, living and enabled
		};

		enum class Answer { kLetIn, kNobody, kTooLate };

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
		bool g_choiceCanPick = false;
		RE::FormID g_passDoor = 0;      // the next Activate on this door goes to the game untouched
		bool g_simpleActivate = false;
		LetIn g_letIn;

		std::uint32_t g_knocks = 0, g_letIns = 0, g_nobody = 0, g_tooLate = 0, g_promptsShown = 0;
		std::deque<std::string> g_events;

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

			BuildTable();
			for (const auto id : g_actors)
			{
				auto* actor = RE::TESForm::LookupByID<RE::Actor>(id);
				if (!actor || CurrentCell(actor) != cell || actor->IsDisabled() || actor->IsDead() || IsMannequin(actor)) { continue; }
				a_home.residents.push_back(actor);
			}

			if (!owner && a_home.residents.empty()) { return Verdict::kNotAHome; }

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

		// A friend: relationship rank above acquaintance, as the game holds it. The rank levels run Lover (0) .. Friend (3),
		// Acquaintance (4) .. Archnemesis (8), so "above acquaintance" is a smaller number.
		bool IsFriend(RE::TESNPC* a_npc)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* playerBase = player ? player->GetActorBase() : nullptr;
			if (!a_npc || !playerBase) { return false; }
			const auto* relationship = RE::BGSRelationship::GetRelationship(a_npc, playerBase);
			return relationship && relationship->level.underlying() < static_cast<std::underlying_type_t<RE::BGSRelationship::RELATIONSHIP_LEVEL>>(RE::BGSRelationship::RELATIONSHIP_LEVEL::kAcquaintance);
		}

		bool FriendInside(const Home& a_home)
		{
			for (auto* actor : a_home.residents)
			{
				if (IsFriend(actor->GetActorBase())) { return true; }
			}
			return false;
		}

		Answer Decide(const Home& a_home)
		{
			if (a_home.residents.empty()) { return Answer::kNobody; }
			if (IsDay() || FriendInside(a_home)) { return Answer::kLetIn; }
			return Answer::kTooLate;
		}

		void ApplyLetIn()
		{
			auto* door = RE::TESForm::LookupByID<RE::TESObjectREFR>(g_letIn.door);
			auto* cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(g_letIn.cell);
			if (auto* lock = door ? door->GetLock() : nullptr) { lock->SetLocked(false); }
			if (cell) { cell->cellFlags.set(RE::TESObjectCELL::Flag::kPublicArea); }
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
			ApplyLetIn();
		}

		// The whole of a knock. True when the door was let open and the game's own Activate should now run.
		bool DoKnock(RE::TESObjectREFR* a_door, const Home& a_home)
		{
			++g_knocks;
			const char* day = IsDay() ? "day" : "night";
			switch (Decide(a_home))
			{
			case Answer::kLetIn:
				++g_letIns;
				BeginLetIn(a_door, a_home);
				Notify(strings::TR("DSK_ComeIn", "Come in."));
				Event(std::format("knock on {:08X} ({}): let in, {} resident(s)", a_door->GetFormID(), day, a_home.residents.size()));
				return true;
			case Answer::kNobody:
				++g_nobody;
				Notify(strings::TR("DSK_NoAnswer", "Nobody answers."));
				Event(std::format("knock on {:08X} ({}): nobody in cell {:08X} now", a_door->GetFormID(), day, a_home.cell->GetFormID()));
				break;
			case Answer::kTooLate:
				++g_tooLate;
				Notify(strings::TR("DSK_TooLate", "It's late. Come back in the morning."));
				Event(std::format("knock on {:08X} ({}): refused, not a friend", a_door->GetFormID(), day));
				break;
			}
			return false;
		}

		// Runs the game's own Activate on the door, past our hook: the door opens if it is open, and the game's lock path runs if not.
		void PassToGame(RE::TESObjectREFR* a_door)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!a_door || !player) { return; }
			g_passDoor = a_door->GetFormID();
			a_door->ActivateRef(player, 0, nullptr, 1, false);
			g_passDoor = 0;
		}

		void OnChoice(std::uint8_t a_button)
		{
			const RE::FormID id = g_choiceDoor;
			const bool canPick = g_choiceCanPick;
			g_choiceDoor = 0;
			auto* tasks = SKSE::GetTaskInterface();
			if (!id || !tasks) { return; }
			// After the box has closed, on the main thread.
			tasks->AddTask([id, canPick, a_button]() {
				auto* door = RE::TESForm::LookupByID<RE::TESObjectREFR>(id);
				if (!door) { return; }
				if (a_button == 0)
				{
					g_tableBuilt = false;   // who is in memory at this moment
					Home home;
					const auto verdict = Classify(door, home);
					if (verdict != Verdict::kHome)
					{
						Event(std::format("knock on {:08X} dropped: {}", id, Name(verdict)));
						return;
					}
					if (DoKnock(door, home)) { PassToGame(door); }
				}
				else if (a_button == 1 && canPick)
				{
					Event(std::format("player chose to pick {:08X}", id));
					PassToGame(door);
				}
			});
		}

		bool ShowChoice(RE::TESObjectREFR* a_door, const Home& a_home)
		{
			// Signature from CommonLibSSE-NG 7.2's MessageBoxMenu::Create, the same function on every runtime: it reads
			// the buttons up to a nullptr and hands the callback the button's index plus the offset.
			using func_t = bool(const char*, void (*)(std::uint8_t), std::uint8_t, std::int32_t, std::int32_t, ...);
			static REL::Relocation<func_t> create{ RELOCATION_ID(51420, 52269) };

			const char* place = a_home.cell ? a_home.cell->GetFullName() : nullptr;
			if (!place || !place[0]) { place = a_door->GetDisplayFullName(); }
			const std::string body = std::format("{}\n{}", place ? place : "", strings::TR("DSK_ChoiceBody", "The door is locked."));

			g_choiceDoor = a_door->GetFormID();
			g_choiceCanPick = a_door->GetLockLevel() != RE::LOCK_LEVEL::kRequiresKey;
			const char* knockText = strings::TR("DSK_Knock", "Knock");
			const char* pickText = strings::TR("DSK_ChoicePick", "Pick the lock");
			const char* leaveText = strings::TR("DSK_ChoiceLeave", "Leave");
			// A key-only lock cannot be picked, so it has no such button; Leave then sits where a press does nothing.
			const bool shown = g_choiceCanPick ? create(body.c_str(), OnChoice, std::uint8_t{ 0 }, 0, 10, knockText, pickText, leaveText, nullptr) :
			                                     create(body.c_str(), OnChoice, std::uint8_t{ 0 }, 0, 10, knockText, leaveText, nullptr);
			if (!shown)
			{
				g_choiceDoor = 0;
				logger::warn("the choice box for {:08X} did not open", a_door->GetFormID());
			}
			return shown;
		}

		struct DoorHooks
		{
			// CommonLibSSE-NG names this argument a_activator. It must be the door's reference for this to do anything, so it
			// is checked against this door's own form, and anything else takes the game's own text.
			static bool GetActivateText(RE::TESObjectDOOR* a_this, RE::TESObjectREFR* a_ref, RE::BSString& a_dst)
			{
				if (a_ref && a_ref->GetBaseObject() == a_this)
				{
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
				if (a_target && a_activator && a_activator->IsPlayerRef() && a_target->GetFormID() != g_passDoor && a_target->GetFormID() == g_promptDoor)
				{
					Home home;
					if (Classify(a_target, home) == Verdict::kHome && ShowChoice(a_target, home))
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
	}

	void OnGameLoaded()
	{
		g_tableBuilt = false;
		g_promptDoor = 0;
		g_choiceDoor = 0;
		g_passDoor = 0;
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
		return std::format(
			"{{\"ok\":true,\"enabled\":{},\"dayStart\":{},\"dayEnd\":{},\"hour\":{:.2f},\"isDay\":{},\"simpleActivate\":{},\"promptDoor\":\"{:08X}\",\"choiceDoor\":\"{:08X}\","
			"\"letIn\":{{\"door\":\"{:08X}\",\"cell\":\"{:08X}\",\"cellWasPublic\":{}}},"
			"\"table\":{{\"built\":{},\"actors\":{},\"ms\":{}}},"
			"\"exclusions\":{{\"forms\":{},\"editorIDs\":{},\"unresolved\":{}}},"
			"\"counts\":{{\"prompts\":{},\"knocks\":{},\"letIn\":{},\"nobody\":{},\"tooLate\":{}}},\"events\":[{}]}}",
			settings::enabled, settings::dayStartHour, settings::dayEndHour, calendar ? calendar->GetHour() : -1.0F, IsDay(), g_simpleActivate, g_promptDoor, g_choiceDoor,
			g_letIn.door, g_letIn.cell, g_letIn.cellWasPublic,
			g_tableBuilt, g_tableActors, g_tableMs,
			g_excludedIDs.size(), g_excludedEditorIDs.size(), g_excludedUnresolved,
			g_promptsShown, g_knocks, g_letIns, g_nobody, g_tooLate, events);
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
			residents += std::format("{}{{\"id\":\"{:08X}\",\"name\":\"{}\",\"friend\":{}}}", residents.empty() ? "" : ",", actor->GetFormID(), Escape(actor->GetDisplayFullName()), IsFriend(actor->GetActorBase()));
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
		const char* answer = verdict != Verdict::kHome ? "none" : (Decide(home) == Answer::kLetIn ? "let in" : (Decide(home) == Answer::kNobody ? "nobody" : "too late"));
		return std::format(
			"{{\"ok\":true,\"verdict\":\"{}\",\"answer\":\"{}\",\"cell\":\"{:08X}\",\"cellAttached\":{},\"owner\":\"{:08X}\",\"ownerType\":\"{}\",\"residents\":[{}],\"ownerPeople\":[{}]}}",
			Name(verdict), answer, home.cell ? home.cell->GetFormID() : 0, home.cell ? home.cell->IsAttached() : false, home.owner ? home.owner->GetFormID() : 0,
			home.ownerNPC ? "npc" : (home.ownerFaction ? "faction" : (home.owner ? "other" : "none")), residents, people);
	}

	std::string KnockJson(std::uint32_t a_refID)
	{
		auto* door = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_refID);
		if (!door) { return std::format("{{\"ok\":false,\"error\":\"no reference {:08X}\"}}", a_refID); }
		Home home;
		const auto verdict = Classify(door, home);
		if (verdict != Verdict::kHome) { return std::format("{{\"ok\":false,\"verdict\":\"{}\"}}", Name(verdict)); }
		if (DoKnock(door, home)) { PassToGame(door); }
		return std::format("{{\"ok\":true,\"state\":{}}}", StateJson());
	}

	std::string ChoiceJson(std::uint32_t a_refID)
	{
		auto* door = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_refID);
		if (!door) { return std::format("{{\"ok\":false,\"error\":\"no reference {:08X}\"}}", a_refID); }
		Home home;
		const auto verdict = Classify(door, home);
		if (verdict != Verdict::kHome) { return std::format("{{\"ok\":false,\"verdict\":\"{}\"}}", Name(verdict)); }
		return std::format("{{\"ok\":{}}}", ShowChoice(door, home));
	}

	std::string EndLetInJson()
	{
		EndLetIn("DevBench");
		return R"({"ok":true,"op":"endletin"})";
	}
}
