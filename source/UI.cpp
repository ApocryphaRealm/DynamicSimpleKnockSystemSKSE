#include "PCH.h"

#include "UI.h"

#include "SKSEMenuFramework.h"

#include "Knock.h"
#include "PreciseSlider.h"
#include "Settings.h"

#include "utils/Logger.h"
#include "utils/Strings.h"
#include "utils/Toggle.h"

#include <algorithm>
#include <functional>
#include <string>

namespace UI
{
	namespace
	{
		std::string statusMessage;
		std::string selectedSlider;

		void OnMainThread(std::function<void()> a_task)
		{
			if (auto* taskInterface = SKSE::GetTaskInterface()) { taskInterface->AddTask(std::move(a_task)); }
		}

		bool HasRequiredExports()
		{
			constexpr const char* required[] = {
				"AddSectionItem",
				"igTextV",
				"igTextDisabledV",
				"igTextWrappedV",
				"igSetTooltipV",
				"igSeparatorText",
				"igSliderInt",
				// precise::SliderInt (include/PreciseSlider.h, rule 68) asks these to tell a nudge from a drag.
				"igIsKeyDown_Nil",
				"igIsMouseDown_Nil",
				"igNavMoveRequestCancel",
				"igIsKeyPressed_Bool",
				"igIsItemClicked",
				"igIsItemActive",
				"igIsItemHovered",
				"igButton",
				"igSameLine",
				"igSpacing",
				"igPushItemWidth",
				"igPopItemWidth",
				"igGetCursorScreenPos",
				"igGetWindowDrawList",
				"igGetFrameHeight",
				"igInvisibleButton",
				"igPushID_Str",
				"igPopID",
				"ImDrawList_AddRectFilled",
				"ImDrawList_AddCircleFilled"
			};
			for (const char* name : required)
			{
				if (!GetMenuFrameworkFunction<void*>(name))
				{
					logger::warn("The menu framework does not export \"{}\"", name);
					return false;
				}
			}
			return true;
		}

		void HelpMarker(const char* a_description)
		{
			ImGuiMCP::SameLine();
			ImGuiMCP::TextDisabled("%s", strings::TR("DSK_HelpMark", "(?)"));
			if (ImGuiMCP::IsItemHovered()) { ImGuiMCP::SetTooltip("%s", a_description); }
		}

		// One whole hour per nudge, by arrow key or D-pad (rule 68).
		bool HourSlider(const char* a_label, int* a_value)
		{
			bool changed = precise::SliderInt(a_label, a_value, 0, 24, "%d");
			if (ImGuiMCP::IsItemClicked() || ImGuiMCP::IsItemActive()) { selectedSlider = a_label; }
			if (selectedSlider == a_label)
			{
				int nudge = 0;
				if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_LeftArrow) || ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_DownArrow)) { nudge -= 1; }
				if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_RightArrow) || ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_UpArrow)) { nudge += 1; }
				if (nudge != 0 && !changed)
				{
					*a_value = std::clamp(*a_value + nudge, 0, 24);
					changed = true;
				}
				if (nudge != 0) { ImGuiMCP::NavMoveRequestCancel(); }
			}
			return changed;
		}

		// One whole unit per nudge (rule 68), with the same press-to-select as the hour sliders.
		void IntSlider(const char* a_label, int* a_value, int a_min, int a_max, const char* a_helpKey, const char* a_help)
		{
			precise::SliderInt(a_label, a_value, a_min, a_max, "%d");
			HelpMarker(strings::TR(a_helpKey, a_help));
		}

		void RenderButtons()
		{
			ImGuiMCP::SeparatorText("");

			if (ImGuiMCP::Button(strings::TR("DSK_SaveBtn", "Save")))
			{
				statusMessage = strings::TR("DSK_StatusSaving", "Saving...");
				OnMainThread([]() {
					statusMessage = settings::Save() ? strings::TR("DSK_StatusSaved", "Settings saved.")
													 : strings::TR("DSK_StatusSaveFail", "Could not write the INI. See the log for why.");
				});
			}
			HelpMarker(strings::TR("DSK_HelpSave", "Writes this page to the mod's INI so it survives a restart."));

			ImGuiMCP::SameLine();
			if (ImGuiMCP::Button(strings::TR("DSK_ReloadBtn", "Reload from INI")))
			{
				statusMessage = strings::TR("DSK_StatusReloading", "Reloading...");
				OnMainThread([]() {
					const bool ok = settings::Reload();
					knock::ResolveExclusions();
					knock::RefreshPrompt();
					statusMessage = ok ? strings::TR("DSK_StatusReloaded", "Settings reloaded from the INI.")
									   : strings::TR("DSK_StatusReloadFail", "Could not read the INI. See the log for why.");
				});
			}
			HelpMarker(strings::TR("DSK_HelpReload", "Throws away changes made here since the last save, and reads the INI again, exclusions too."));

			ImGuiMCP::SameLine();
			if (ImGuiMCP::Button(strings::TR("DSK_RestoreBtn", "Restore defaults")))
			{
				OnMainThread([]() {
					settings::RestoreDefaults();
					knock::RefreshPrompt();
				});
				statusMessage = strings::TR("DSK_StatusRestored", "Defaults restored. Press Save to keep them.");
			}
			HelpMarker(strings::TR("DSK_HelpRestore", "Puts this page back to its fresh-install values. Nothing is written until you press Save."));

			if (!statusMessage.empty()) { ImGuiMCP::TextWrapped("%s", statusMessage.c_str()); }
			ImGuiMCP::Spacing();
			ImGuiMCP::Text("%s", settings::GetIniPath().c_str());
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled())
		{
			logger::info("No menu framework is installed; settings are read from the INI only");
			return;
		}
		if (!HasRequiredExports())
		{
			logger::warn("The installed menu framework is older than this page needs. Settings are read from the INI only.");
			return;
		}
		SKSEMenuFramework::SetSection("Dynamic Simple Knock System SKSE");
		SKSEMenuFramework::AddSectionItem("Settings", SettingsPanel::Render);
		logger::info("Registered the settings page with the menu framework");
	}

	void __stdcall SettingsPanel::Render()
	{
		strings::Tick();

		ImGuiMCP::TextWrapped("%s", strings::TR("DSK_Intro", "Changes apply as soon as you make them. Press Save to keep them for the next time you play."));
		ImGuiMCP::Spacing();

		ImGuiMCP::PushItemWidth(260.0F);

		ImGuiMCP::SeparatorText(strings::TR("DSK_General", "Knocking"));
		bool enabled = settings::enabled;
		if (ImGuiMCP::Toggle(strings::TR("DSK_Enable", "Enable knocking"), &enabled))
		{
			settings::enabled = enabled;
			OnMainThread([]() { knock::RefreshPrompt(); });
		}
		HelpMarker(strings::TR("DSK_HelpEnable", "Off: every door behaves as it does without this mod."));

		HourSlider(strings::TR("DSK_DayStart", "Day starts at"), &settings::dayStartHour);
		HelpMarker(strings::TR("DSK_HelpDayStart", "From this hour the night penalty stops. 24-hour clock."));
		HourSlider(strings::TR("DSK_DayEnd", "Night starts at"), &settings::dayEndHour);
		HelpMarker(strings::TR("DSK_HelpDayEnd", "From this hour the night penalty applies. 24-hour clock."));

		ImGuiMCP::SeparatorText(strings::TR("DSK_Decision", "Who lets you in"));
		IntSlider(strings::TR("DSK_FriendRank", "Always open to rank"), &settings::friendRank, 1, 4, "DSK_HelpFriendRank",
			"From this relationship rank a knock is always answered: 1 friend, 2 confidant, 3 ally, 4 lover.");
		IntSlider(strings::TR("DSK_KnockSpeech", "Speech for a knock"), &settings::knockSpeech, 0, 100, "DSK_HelpKnockSpeech",
			"The Speech skill a plain knock needs from a stranger, before the penalties.");
		IntSlider(strings::TR("DSK_PersuadeSpeech", "Speech to persuade"), &settings::persuadeSpeech, 0, 100, "DSK_HelpPersuadeSpeech",
			"The Speech skill Persuade needs, before the same penalties.");
		IntSlider(strings::TR("DSK_NightPenalty", "Night penalty"), &settings::nightPenalty, 0, 100, "DSK_HelpNightPenalty", "Added to the Speech needed at night.");
		IntSlider(strings::TR("DSK_AsleepPenalty", "Woken penalty"), &settings::asleepPenalty, 0, 100, "DSK_HelpAsleepPenalty", "Added to the Speech needed when the one answering was asleep.");
		IntSlider(strings::TR("DSK_BountyGold", "Bounty that counts"), &settings::bountyGold, 0, 5000, "DSK_HelpBountyGold",
			"A bounty of this much gold or more in their hold adds the bounty penalty. 0: bounties do not count.");
		IntSlider(strings::TR("DSK_BountyPenalty", "Bounty penalty"), &settings::bountyPenalty, 0, 100, "DSK_HelpBountyPenalty", "Added to the Speech needed when your bounty counts.");
		IntSlider(strings::TR("DSK_ThiefGold", "Theft that counts"), &settings::thiefGold, 0, 5000, "DSK_HelpThiefGold",
			"Seen stealing goods worth this much gold or more in their hold adds the thief penalty. 0: thefts do not count.");
		IntSlider(strings::TR("DSK_ThiefPenalty", "Thief penalty"), &settings::thiefPenalty, 0, 100, "DSK_HelpThiefPenalty", "Added to the Speech needed when you are a known thief there.");
		IntSlider(strings::TR("DSK_DislikePenalty", "Dislike penalty"), &settings::dislikePenalty, 0, 100, "DSK_HelpDislikePenalty", "Added for each relationship rank below acquaintance.");
		IntSlider(strings::TR("DSK_IntimidateMargin", "Intimidate margin"), &settings::intimidateMargin, -50, 50, "DSK_HelpIntimidateMargin",
			"How far your level, raised by your Speech, must beat their level scaled by their confidence. Negative makes it easier.");
		IntSlider(strings::TR("DSK_RetryHours", "Hours before a new try"), &settings::retryHours, 0, 72, "DSK_HelpRetryHours",
			"Game hours after a refusal before a knock starts over, with Persuade and Intimidate offered again.");

		ImGuiMCP::SeparatorText(strings::TR("DSK_Sound", "Sound"));
		IntSlider(strings::TR("DSK_VoiceVolume", "Answer volume"), &settings::voiceVolume, 0, 100, "DSK_HelpVoiceVolume",
			"How loud the answer through the door is, in percent of the game's own loudness. 100: as the game plays it.");

		ImGuiMCP::PopItemWidth();
		ImGuiMCP::Spacing();

		ImGuiMCP::TextDisabled("%s", strings::TR("DSK_ExclusionsNote", "Doors to leave alone are listed in the INI, under [Exclusions]."));

		RenderButtons();
	}
}
