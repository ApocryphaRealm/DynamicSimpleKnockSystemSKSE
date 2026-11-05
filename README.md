# Dynamic Simple Knock System SKSE

Look at the locked door of somebody's home and the prompt says Knock. Press your activate control. By day they let you in with a "Come in." At night only a friend gets in. A refusal changes nothing: the door stays locked, its own prompt comes back so you can pick it, and sneaking always gives you the lock.

No plugin, no scripts, no patches. It reads who owns the door or the space behind it while you play, so a door from any mod works. Nobody needs to carry a key.

Requires SKSE and Address Library. Skyrim SE 1.5.97, AE 1.6.1170, and 1.7.99 / 1.7.104. A page in the Apocrypha Menu Framework or SKSE Menu Framework is optional; without one the mod runs from its INI.

## The rule

A locked door in an exterior cell whose teleport leads to an interior, where the door, its other half or the destination cell has an owner (an NPC or a faction), or where a living resident is placed.

Left alone: you sneaking or in combat, you holding the key, your own home, destination locations with a dungeon keyword, anybody there hostile to you, and the `[Exclusions]` list in the INI. Doors between two interiors are left alone too: most are jarls' quarters, temple sanctums and vaults.

## Who answers

- A resident is the owner's actor, or any actor placed in the cell, who is alive and enabled. Mannequins are not residents.
- A resident who is elsewhere at the moment still answers. This is a first-version choice.
- An actor exists in memory while its cell is unloaded only when it is persistent. A non-persistent resident is not seen. A unique NPC owner with no actor in memory counts as alive unless the game's dead count for that NPC says otherwise.
- A faction owner with nobody found: still someone's home. It answers by the day rule, and at night by your standing with that faction (its reaction to the player is ally or friend).
- Everybody placed there is dead: "Nobody answers."
- A friend is a relationship rank above acquaintance, read from the game.

## Being let in

The door's lock is opened and the cell's Public flag set in memory only, with no change flag on either form, so the game's save carries nothing of it. One let-in exists at a time, kept in a single SKSE co-save record, so a save made inside still works. It ends the next time a load finishes with you in an exterior cell: the door locks again and the cell returns to what it was.

## Conflicts

Simpler Knock and its patches do the same job through a perk. Use one or the other.

## Building

Visual Studio with C++, and `VCPKG_ROOT` pointing at a vcpkg checkout. `configure.bat` then `build.bat` for SE 1.5.97 and AE 1.6.1170; `configure17.bat` then `build17.bat` for 1.7. `py -3 tools/make_translations.py` writes the eleven language files.

## Licence

GPL-3.0-or-later. See `NOTICE.md` and `THIRD_PARTY_NOTICES.md`.
