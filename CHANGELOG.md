# DynamicSimpleKnock - changelog

Written as changes happen, not reconstructed afterwards (rule 61). Each version carries its
**version-ledger status**, so this file cannot quietly claim more than the ledger does:

* **working** - observed running in game
* **untested** - built and packaged, not yet confirmed
* **failed** - built but crashed or malfunctioned; the number was reclaimed
* **scratch** - a hypothesis-test build that never held a real number

## 1.3.2 - 2026-11-12 - untested

### Fixed
- Inns and other public places are no longer treated as homes. When another mod locks an inn (or a temple, keep, guild hall or jail), Knock now steps aside: no Knock prompt, no box, just the game's own lock. A place counts as public when its location is an inn, or when the game marks the cell as a public area; homes that the game also marks public (Nepos's House, Silus's House, the player houses and the like) are still homes. Shops are still homes: their owners live there and lock up at night.
- Leaving a house you were let into locks the front door again only if Knock unlocked it and nobody has changed it since. A door that another mod or the console unlocked, locked or changed meanwhile is left as it is.
- The way out of a house you were let into no longer opens a lock that another mod or the console put on the door. Knock still opens the door's own lock - the one its plugin gives it, which the household locks at night - so you can always walk out of a house you were let into. How it tells them apart: when the game loads, Knock notes every door's lock level as the plugins set it. On the way out, a locked door is opened only if its plugin gives it a lock and the level is still the plugin's. A lock on a door whose plugin has none, or at another level, was added later by a mod or the console and is left alone, with a log line saying so. (A console lock on the outside door also locks the inside one, which Knock used to open.)

## 1.3.1 - 2026-11-11 - working

### Fixed
- Pick the lock from the knock box hands the door to the player's own activation, so lockpicking mods that work through an activate perk (ESO Lockpick Menu) open their lockpicking.
- Knock now works with lockpicking mods that take the Activate press with a perk (ESO Lockpick Menu and the like): the press on a Knock door opens the knock box first; Pick the lock still opens the lockpick menu.

## 1.3.0 - 2026-11-09 - working

### Added
- Sharp only: the end of the Labyrinthian door knock, its two sharp knocks without the 1.1 s of stone grinding before them, as the mod's own loose sound file (Sound\FX\DynamicSimpleKnock\DSK_Knock_Sharp.wav, cut from the game's file 5 ms before its first knock with a 5 ms fade-in), played by file path. No plugin.
- Wood knock (the idea and the timings are kialiva's, from the mod's Nexus comments): you bang on the door with the game's IdleMT_DoorBang and the game's wooden footstep sound FSTPlayerWoodBarefootSprintL knocks at the door 1450, 1850, 4550, 4850 and 5150 ms after the bang starts; moving away, drawing or sheathing, another idle or a cell change cancels the knocks still to come. No plugin: both are Skyrim.esm records.
- Knock sound setting: [Sound] iKnockSound and a Knock sound slider on the settings page, in all eleven languages: wood / sharp only / full / off, default wood.

### Changed
- The answer to a knock waits for the knock to finish: in wood mode until the last knock plus a short beat (5.9 s), in sharp only and full until the sound ends; off answers at once. Walking away from the door (or interrupting the wood knock) cancels the answer, and a knock pressed while one is still sounding is ignored.
- The knock no longer sounds like kicking a huge metal door on a wooden house door (reported on Nexus): the default is now the wood knock; the old sound is still there as full.

## 1.2.0 - 2026-11-08 - working

### Fixed
- Knock is reachable on locked homes with a perk that adds its own activate choice to locked doors (Ordinator's Bypass Lock, reported on Nexus): the game's choice box comes first, and its default door activation now opens the knock box; the perk's own choice works as before.

### Added
- Answer volume: [Voice] iVoiceVolume (0 to 100, default 100 = the game's own loudness) turns the resident's spoken answer down; also a slider on the settings page, in all eleven languages.
- A knock sound at the door when you knock: Skyrim.esm's door-knock sound descriptor (DRSStoneLabyrinthianKnock), set in the INI under [Sound] sKnockSound (empty for none).

## 1.1.0 - 2026-11-07 - working

### Added
- Persuade and Intimidate answer in their own voice lines, the game's generic speech-check answers: persuaded (Okay. Just this once. / How can I argue with that? / Okay, okay. You've talked me into it.), not persuaded (You expect me to believe that? / Nah. I don't think so. / That doesn't change anything.), intimidated (No need to get rough! / Whatever you say! / Don't hurt me! You win.), not intimidated (I don't have to take that from you! / I don't have to take this from the likes of you.). A voice type without these lines (a DLC or mod voice) still says the let-in or refusal line.
- Peek through the keyhole: with Take a Peek installed, the knock box (and the refusal box when there is room) offers its peek; without it the choice is not shown

### Fixed
- The Peek choice's label is in the eleven translation files (it drew English in every language; the finalize gate's translation coverage found it). Text files only; the DLLs are unchanged.
- Let in, the household no longer says You're not supposed to be here. on entry or when you talk to them, and talking to them opens their normal conversation (owner's test 2026-11-07): the game still counted you as trespassing in the house; while you are inside the house you were let into, that state is cleared and the trespassing conversation is silenced. Their belongings stay theirs: taking them is still theft.
- No more being told to leave once you are let in (test 2026-11-07: a resident kept saying You need to leave.): from the moment the door is unlocked for you until it locks again, the game's own get-out lines (Time To Go and the trespass warnings, from any plugin) are silenced, including on the way in.
- No report to the guards once you are let in: while a let-in by Knock, Persuade or Intimidate holds and you are in that house, nobody can start the trespass warnings that end in a trespass crime and bounty, residents and guards alike, on top of the house being treated as public. Stealing and other crimes are still crimes.
- Locked in from the inside (Nexus report, 2026-11-07): while a let-in holds, a door of that house that leads outside and has been locked by the game (a resident's AI package locks their home as they leave or go to bed; 161 vanilla homes carry a lock on the inside door) is unlocked in memory when you look at it or press it, so you can always walk out without a lockpick. The mod itself never locked the inside door; the outside door still locks again once you are outside.
- Nobody home, nobody answers (Nexus report, 2026-11-07): a resident out in town, at work or in another cell no longer answers and lets you in. Only a living, enabled resident inside the house now answers; the 1.0.0 widening to residents placed there and to the owner or owning faction wherever they were is gone. Nobody inside: the box still offers Knock, Pick the lock and Peek; a knock shows No one's home. Come back later. (eleven languages), with no spoken line, no let-in and no Persuade or Intimidate. A refusal box from earlier is shown only while the one who refused is still inside.

## 1.0.0 - 2026-11-06 - working

### Added
- Settings page: a Who lets you in section with every threshold as a precise slider, in the INI under [Decision]. All new choices and messages in the eleven languages. DevBench dsk.control: op=set and op=clear for the decision inputs (speech, rank, levels, confidence, bounty, stolen, night, asleep), op=persuade, op=intimidate, op=refusalbox; op=check and op=knock answer with the decision's inputs and result.
- Refused, the box offers Persuade (your Speech against iPersuadeSpeech 30 plus the same penalties) and Intimidate (your level raised by your Speech, against their level scaled by their confidence through the game's own fIntimidateConfidenceMult settings), once each per refusal, with Pick the lock and Leave. Success: a voiced agreement and the door unlocks. Failure: a voiced refusal, the door stays locked. A new knock starts over after iRetryHours (1).
- The resident answers aloud in their own voice, with no plugin: the native Say(topic, speak as the resident) plays a vanilla DialogueGeneric line at the door - let in 039C2A (I'll let you stay. This time.), refused 06AEA2 (You need to leave.). Both topics are INI settings under [Voice].
- Settings page: enable, day start, night start, precise sliders, eleven languages. DevBench tool dsk.control.
- Letting in holds no change in the game's save: lock and the cell's Public flag are set in memory, with one co-save record for a save made inside. It ends when a load finishes outside, and the door locks again.
- Read at run time from the door's and the destination cell's owner and the residents placed there: no plugin, no patches, no key needed. Dungeon keywords, hostile actors, the player's own homes and the INI's [Exclusions] list are left out.
- Knock on the locked door of somebody's home: the prompt says Knock, your activate control knocks, on keyboard and controller. By day they let you in, at night only a friend does. A refusal leaves the door locked and its own prompt back so it can be picked; sneaking always gives the lock.

### Known
- Voice types the let-in topic has no line for (young eager, female soldier, guard, bandit, Forsworn, warlock) say nothing; the on-screen message still says what happened. Whether Say speaks as a resident whose cell is not loaded is not yet proven in game.
- Non-persistent residents of an unloaded cell are not seen, so their door says Nobody answers. Whether an unloaded persistent actor's current cell follows their schedule is not yet proven. Only the door's destination cell is made public, not a second cell behind it. Not used together with Simpler Knock.
### Changed
- Knock no longer opens the door by itself (the owner, 2026-11-06: they have to agree to let you in). The resident with the best relationship to you answers and decides: a friend or better always lets you in; anyone else when your Speech reaches iKnockSpeech (50) plus penalties for night (15), for waking them (15), for a bounty in their hold (25), for being seen stealing there (20) and per rank of dislike (15). A sleeping resident answers too. A let-in unlocks the door and your next press opens it, so the spoken line is not cut off by the loading screen.
- A knock is answered only by somebody in the destination cell right now, by the actor's current cell (the owner, 2026-11-05). Nobody there: Nobody answers. This replaces residents elsewhere answering and the faction standing fallback. DevBench op=check lists the owner's people with where the game holds them.
- A press on a locked home door now shows a box: Knock, Pick the lock, Leave. A lock is only picked when chosen. First test: a refused door's second press reached the lock path, Lock Interaction Overhaul's auto-pick took it with its trespass alarm, and the player was jailed; the let-in never ran for that door.
### Fixed
- Nobody answered at the House of Clan Cruel-Sea (test, 2026-11-06 07:05): its three persistent residents were held elsewhere by their schedules. The search now widens in order and stops at the first that finds anybody: in the cell now, then living there (their editor cell is that cell) wherever the game holds them, then the owning NPC or a member of the owning faction. Alive, enabled and not hostile as before; the log and op=check name the method. Before Say, the topic is checked for a line whose conditions pass for that speaker; with none, the message alone shows, and the log says so.
- The prompt shows under Simple Activate, which drops the first line of a door's prompt: while it is loaded the prompt is one line, Knock: name.