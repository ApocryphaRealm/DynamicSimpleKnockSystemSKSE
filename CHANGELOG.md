# DynamicSimpleKnock - changelog

Written as changes happen, not reconstructed afterwards (rule 61). Each version carries its
**version-ledger status**, so this file cannot quietly claim more than the ledger does:

* **working** - observed running in game
* **untested** - built and packaged, not yet confirmed
* **failed** - built but crashed or malfunctioned; the number was reclaimed
* **scratch** - a hypothesis-test build that never held a real number

## 1.1.0 - 2026-11-07 - untested

### Added
- Persuade and Intimidate answer in their own voice lines, the game's generic speech-check answers: persuaded (Okay. Just this once. / How can I argue with that? / Okay, okay. You've talked me into it.), not persuaded (You expect me to believe that? / Nah. I don't think so. / That doesn't change anything.), intimidated (No need to get rough! / Whatever you say! / Don't hurt me! You win.), not intimidated (I don't have to take that from you! / I don't have to take this from the likes of you.). A voice type without these lines (a DLC or mod voice) still says the let-in or refusal line.
- Peek through the keyhole: with Take a Peek installed, the knock box (and the refusal box when there is room) offers its peek; without it the choice is not shown

### Fixed
- No more being told to leave once you are let in (test 2026-11-07: a resident kept saying You need to leave.): while you are inside the house you were let into, the game's own get-out lines (Time To Go and the trespass warnings, from any plugin) are silenced; they come back as soon as you are anywhere else.
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