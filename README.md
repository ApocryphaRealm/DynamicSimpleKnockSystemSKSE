# Dynamic Simple Knock System SKSE

Look at the locked door of somebody's home and the prompt says Knock. Press your activate control and a box offers Knock, Pick the lock, or Leave. Knock, and whoever lives there answers from behind the door, aloud in their own voice, and decides whether to let you in. Refused, you may Persuade or Intimidate them, pick the lock, or leave. A lock is only picked when you choose it, and sneaking goes straight to the lock.

No plugin, no scripts, no patches. It reads who owns the door or the space behind it while you play, so a door from any mod works. Nobody needs to carry a key.

Requires SKSE and Address Library. Skyrim SE 1.5.97, AE 1.6.1170, and 1.7.99 / 1.7.104. A page in the Apocrypha Menu Framework or SKSE Menu Framework is optional; without one the mod runs from its INI.

## The rule

A locked door in an exterior cell whose teleport leads to an interior, where the door, its other half or the destination cell has an owner (an NPC or a faction), or where a living resident is placed.

Left alone: you sneaking or in combat, you holding the key, your own home, destination locations with a dungeon keyword, anybody there hostile to you, and the `[Exclusions]` list in the INI. Public places are left to the game too: a door into an inn (location type LocTypeInn), or into a cell its plugin marks PublicArea (temples, keeps, guild halls, jails), shows no Knock even when another mod has locked it, and the game's own lock applies. A house the game also marks PublicArea (location type LocTypeHouse) is still a home; shops are homes too. Doors between two interiors are left alone too: most are jarls' quarters, temple sanctums and vaults.

## Who answers

The search widens in order and stops at the first that finds anybody: somebody in the destination cell now; a resident who lives there (their editor cell is that cell), wherever the game holds them; the owning NPC or a member of the owning faction. Alive, enabled, not hostile. The one with the best relationship to you answers, adults before children, the awake before the asleep. Nobody found: "Nobody answers." A non-persistent resident of an unloaded cell is not seen.

## Who lets you in

A friend or better (relationship rank `iFriendRank`, 1) always lets you in. Anyone else when your Speech reaches `iKnockSpeech` (50) plus 15 at night, 15 if they were asleep, 25 with a bounty in their hold, 20 if you were seen stealing there, and 15 per rank of dislike. Refused, Persuade needs `iPersuadeSpeech` (30) plus the same additions; Intimidate compares your level times (1 + Speech/100) against theirs scaled by the game's fIntimidateConfidenceMult settings. Each once per refusal; a new knock starts over after `iRetryHours` (1). Every number is in the INI under `[Decision]` and on the settings page.

The answer is spoken from the door with the native `ObjectReference.Say(topic, speakAs)`: vanilla DialogueGeneric topics (`[Voice]`), so the resident's voice type picks the line. A voice type with no line in the topic answers by message only. A let-in unlocks the door; the next press opens it.

## Being let in

The door's lock is opened and the cell's Public flag set in memory only, with no change flag on either form, so the game's save carries nothing of it. One let-in exists at a time, kept in a single SKSE co-save record, so a save made inside still works. It ends the next time a load finishes with you in an exterior cell: the door locks again and the cell returns to what it was. The door is locked again only if Knock unlocked it and it is still as Knock left it (unlocked, same lock level); a door another mod or the console changed meanwhile is left as it is.

## The knock sound

`[Sound] iKnockSound`, also on the settings page:

- **Wood** (3, the default): you bang on the door with the game's own door-bang idle (IdleMT_DoorBang) and a wooden knock, the game's FSTPlayerWoodBarefootSprintL sound, sounds at the door five times, 1.45, 1.85, 4.55, 4.85 and 5.15 seconds after the bang starts. Moving away, drawing or sheathing a weapon, or another idle stops the knocks still to come.
- **Sharp only** (1): the end of the game's Labyrinthian door knock, its two sharp knocks without the stone grinding before them, from the mod's own sound file.
- **Full** (2): the game's Labyrinthian door knock whole (`sKnockSound`), stone grinding included.
- **Off** (0): no knock sound.

Every sound is a vanilla record or the mod's own loose file, so this needs no plugin.

## Conflicts

Simple Activate works alongside: it hides the first line of a prompt, so while it is loaded the prompt is one line, "Knock: name". Lock Interaction Overhaul's auto-pick and crime apply to Pick the lock as to any picking.

Simpler Knock and its patches do the same job through a perk. Use one or the other.

## Credit

kialiva, on the mod's Nexus page, for the wood knock: the FSTPlayerWoodBarefootSprintL sound with IdleMT_DoorBang and the five timings.

## Building

Visual Studio with C++, and `VCPKG_ROOT` pointing at a vcpkg checkout. `configure.bat` then `build.bat` for SE 1.5.97 and AE 1.6.1170; `configure17.bat` then `build17.bat` for 1.7. `py -3 tools/make_translations.py` writes the eleven language files.

## Licence

GPL-3.0-or-later. See `NOTICE.md` and `THIRD_PARTY_NOTICES.md`.
