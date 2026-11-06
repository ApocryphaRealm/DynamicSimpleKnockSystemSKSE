Dynamic Simple Knock System SKSE
Version 1.0.0

Look at the locked door of somebody's home and the prompt says Knock. Press your activate control, on keyboard or controller, and a box offers Knock, Pick the lock, or Leave.

Knock, and whoever lives there answers from behind the door, aloud in their own voice, and decides. A friend always lets you in. Anyone else lets you in when your Speech is high enough: 50, plus 15 at night, 15 if you woke them, 25 with a bounty in their hold, 20 if you were seen stealing there, and 15 per rank of dislike. If they agree, the door unlocks and your next press opens it.

Refused, a second box offers Persuade (Speech 30 plus the same additions), Intimidate (your level, raised by your Speech, against theirs scaled by their confidence), Pick the lock and Leave. Persuade and Intimidate once each; a new knock starts over after an hour of game time. A lock is only picked when you choose it, and picking an owned lock is the usual crime. Sneaking goes straight to the lock.

One SKSE DLL: no plugin, no scripts, no patches. It reads who owns the door or the space behind it while you play, so a door from any mod works, and nobody needs to carry a key.

Left alone: your own home, doors you hold the key to, dungeons, places with anybody hostile to you, doors between two interiors, and the [Exclusions] list in the INI.

Do not use it together with Simpler Knock or its patches.

Settings: an optional page in the Apocrypha Menu Framework or SKSE Menu Framework (enable, day hours, and every number under Who lets you in), and the INI, SKSE\Plugins\DynamicSimpleKnock.ini.

Requires SKSE and Address Library. Skyrim SE 1.5.97, AE 1.6.1170, and 1.7.99 / 1.7.104.
