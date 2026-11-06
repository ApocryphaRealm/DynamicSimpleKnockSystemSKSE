# DynamicSimpleKnock - changelog

Written as changes happen, not reconstructed afterwards (rule 61). Each version carries its
**version-ledger status**, so this file cannot quietly claim more than the ledger does:

* **working** - observed running in game
* **untested** - built and packaged, not yet confirmed
* **failed** - built but crashed or malfunctioned; the number was reclaimed
* **scratch** - a hypothesis-test build that never held a real number

## 1.0.0 - 2026-11-05 - untested

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
- The prompt shows under Simple Activate, which drops the first line of a door's prompt: while it is loaded the prompt is one line, Knock: name.