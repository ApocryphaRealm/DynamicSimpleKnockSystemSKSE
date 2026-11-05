# Dynamic Simple Knock System SKSE

Look at the locked door of somebody's home and the prompt says Knock. Press your activate control and a box offers Knock, Pick the lock, or Leave. By day a knock gets you a "Come in." At night only a friend gets in. A refusal changes nothing and the door stays locked. A lock is only picked when you choose it, and sneaking goes straight to the lock.

No plugin, no scripts, no patches. It reads who owns the door or the space behind it while you play, so a door from any mod works. Nobody needs to carry a key.

Requires SKSE and Address Library. Skyrim SE 1.5.97, AE 1.6.1170, and 1.7.99 / 1.7.104. A page in the Apocrypha Menu Framework or SKSE Menu Framework is optional; without one the mod runs from its INI.

## The rule

A locked door in an exterior cell whose teleport leads to an interior, where the door, its other half or the destination cell has an owner (an NPC or a faction), or where a living resident is placed.

Left alone: you sneaking or in combat, you holding the key, your own home, destination locations with a dungeon keyword, anybody there hostile to you, and the `[Exclusions]` list in the INI. Doors between two interiors are left alone too: most are jarls' quarters, temple sanctums and vaults.

## Who answers

Only somebody in the destination cell right now: a living, enabled actor whose current cell is that cell (the owner's choice, 2026-11-05). Nobody there: "Nobody answers." Mannequins do not count. A friend is a relationship rank above acquaintance, read from the game.

An interior that is not loaded holds only its persistent actors in memory, so residents who are not persistent are not seen, and their door says "Nobody answers."

## Being let in

The door's lock is opened and the cell's Public flag set in memory only, with no change flag on either form, so the game's save carries nothing of it. One let-in exists at a time, kept in a single SKSE co-save record, so a save made inside still works. It ends the next time a load finishes with you in an exterior cell: the door locks again and the cell returns to what it was.

## Conflicts

Simple Activate works alongside: it hides the first line of a prompt, so while it is loaded the prompt is one line, "Knock: name". Lock Interaction Overhaul's auto-pick and crime apply to Pick the lock as to any picking.

Simpler Knock and its patches do the same job through a perk. Use one or the other.

## Building

Visual Studio with C++, and `VCPKG_ROOT` pointing at a vcpkg checkout. `configure.bat` then `build.bat` for SE 1.5.97 and AE 1.6.1170; `configure17.bat` then `build17.bat` for 1.7. `py -3 tools/make_translations.py` writes the eleven language files.

## Licence

GPL-3.0-or-later. See `NOTICE.md` and `THIRD_PARTY_NOTICES.md`.
