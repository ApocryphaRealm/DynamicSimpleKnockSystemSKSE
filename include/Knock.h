#pragma once

#include <cstdint>
#include <string>

namespace knock
{
	// The two door hooks and the event sinks. Once, at kDataLoaded.
	void Install();

	// Reads the [Exclusions] list into form IDs. At kDataLoaded and after a settings reload.
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
	std::string CheckJson(std::uint32_t a_refID);   // what the rule says about this door, with the residents it found
	std::string KnockJson(std::uint32_t a_refID);   // the Knock button, without the box
	std::string ChoiceJson(std::uint32_t a_refID);  // opens the choice box for this door
	std::string EndLetInJson();
}
