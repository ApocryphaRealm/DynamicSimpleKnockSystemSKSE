#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace knock
{
	// The two door hooks and the event sinks. Once, at kDataLoaded.
	void Install();

	// Records which interior cells the plugins mark PublicArea, before any save or let-in changes the flag. Once, at kDataLoaded.
	void SnapshotPublicCells();

	// Records every door reference's lock level as the plugins load it, so the way out of a let-in house opens only the
	// plugin's own lock, never one another mod or the console added. Once, at kDataLoaded.
	void SnapshotPluginLocks();

	// Reads the [Exclusions] list into form IDs and the [Voice] topics. At kDataLoaded and after a settings reload.
	void ResolveExclusions();

	// Forgets the actor table so it is built again on the next door. At kPostLoadGame and kNewGame.
	void OnGameLoaded();

	// Has the game ask the door for its prompt again (a setting changed).
	void RefreshPrompt();

	// Co-save: the one door currently let open, if any.
	void OnSave(SKSE::SerializationInterface* a_intfc);
	void OnLoad(SKSE::SerializationInterface* a_intfc);
	void OnRevert(SKSE::SerializationInterface* a_intfc);

	// DevBench, on the game's main thread. Each returns one JSON object.
	std::string StateJson();
	std::string CheckJson(std::uint32_t a_refID);                  // the rule's verdict, the residents, and the decision it would make now
	std::string ActJson(std::uint32_t a_refID, const char* a_act); // "knock", "persuade" or "intimidate", as the box's button does
	std::string ChoiceJson(std::uint32_t a_refID, bool a_refusalBox); // opens the knock box, or the refusal box
	// Decision overrides: a_field("speech") etc. returns the number sent, if any. a_clear drops them all.
	std::string SetJson(const std::function<std::optional<int>(const char*)>& a_field, bool a_clear);
	std::string EndLetInJson();
}
