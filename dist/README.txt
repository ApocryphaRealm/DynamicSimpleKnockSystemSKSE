Dynamic Simple Knock System SKSE
Version 1.1.0

Look at the locked door of somebody's home and the prompt says Knock. Press your activate control, on keyboard or controller, and a box offers Knock, Pick the lock, or Leave. With Take a Peek installed, the box also offers its peek through the keyhole.

Knock, and whoever is in the house right now answers from behind the door, aloud in their own voice, and decides. A friend always lets you in. Anyone else lets you in when your Speech is high enough: 50, plus 15 at night, 15 if you woke them, 25 with a bounty in their hold, 20 if you were seen stealing there, and 15 per rank of dislike. If they agree, the door unlocks and your next press opens it.

Nobody inside: a knock gets "No one's home. Come back later." The box still offers Knock, Pick the lock and Peek, with no Persuade or Intimidate.

Refused, a second box offers Persuade (Speech 30 plus the same additions), Intimidate (your level, raised by your Speech, against theirs scaled by their confidence), Pick the lock and Leave. Persuade and Intimidate once each; a new knock starts over after an hour of game time. Persuaded or not, intimidated or not, the resident answers with a voiced line of its own for each. A lock is only picked when you choose it, and picking an owned lock is the usual crime. Sneaking goes straight to the lock.

Let in, you are a guest: no trespass crime, nobody tells you it is time to go or that you are not supposed to be here, and talking to the household opens their normal conversation. The doors out of the house always open for you while you are let in. Their belongings stay theirs: taking them is still theft.

One SKSE DLL: no plugin, no scripts, no patches. It reads who owns the door or the space behind it while you play, so a door from any mod works, and nobody needs to carry a key.

Left alone: your own home, doors you hold the key to, dungeons, places with anybody hostile to you, doors between two interiors, and the [Exclusions] list in the INI.

Do not use it together with Simpler Knock or its patches.

Knock sound ([Sound] iKnockSound, also on the settings page): Wood, the default, bangs on the door with the game's door-bang idle while a wooden knock sounds five times; moving away or drawing a weapon stops it. Sharp only plays the two sharp knocks at the end of the game's Labyrinthian door knock, from the mod's own sound file. Full plays that whole sound, stone grinding included. Off is silent. No plugin either way.

Credit: kialiva, on the mod's Nexus page, for the wood knock (the sound, the idle and the five timings).

Settings: an optional page in the Apocrypha Menu Framework or SKSE Menu Framework (enable, day hours, and every number under Who lets you in), and the INI, SKSE\Plugins\DynamicSimpleKnock.ini.

Requires SKSE and Address Library. Skyrim SE 1.5.97, AE 1.6.1170, and 1.7.99 / 1.7.104.

What changed

Version 1.1.0
Peek through the keyhole with Take a Peek, from the knock box and the refusal box.
Only someone in the house right now answers; with nobody home a knock gets "No one's home. Come back later."
The doors out of a house you were let into always open.
Persuade and Intimidate outcomes have their own voice lines.
Let in, there is no trespass crime, no "time to go" or "not supposed to be here" lines, and the household talks to you normally.

Version 1.0.0
First release.
