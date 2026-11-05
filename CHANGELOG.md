# DynamicSimpleKnock - changelog

Written as changes happen, not reconstructed afterwards (rule 61). Each version carries its
**version-ledger status**, so this file cannot quietly claim more than the ledger does:

* **working** - observed running in game
* **untested** - built and packaged, not yet confirmed
* **failed** - built but crashed or malfunctioned; the number was reclaimed
* **scratch** - a hypothesis-test build that never held a real number

## 1.0.0 - 2026-11-05 - untested

### Added
- Settings page: enable, day start, night start, precise sliders, eleven languages. DevBench tool dsk.control.
- Letting in holds no change in the game's save: lock and the cell's Public flag are set in memory, with one co-save record for a save made inside. It ends when a load finishes outside, and the door locks again.
- Read at run time from the door's and the destination cell's owner and the residents placed there: no plugin, no patches, no key needed. Dungeon keywords, hostile actors, the player's own homes and the INI's [Exclusions] list are left out.
- Knock on the locked door of somebody's home: the prompt says Knock, your activate control knocks, on keyboard and controller. By day they let you in, at night only a friend does. A refusal leaves the door locked and its own prompt back so it can be picked; sneaking always gives the lock.

### Known
- Non-persistent residents of an unloaded cell are not seen, so their door says Nobody answers. Whether an unloaded persistent actor's current cell follows their schedule is not yet proven. Only the door's destination cell is made public, not a second cell behind it. Not used together with Simpler Knock.
### Changed
- A knock is answered only by somebody in the destination cell right now, by the actor's current cell (the owner, 2026-11-05). Nobody there: Nobody answers. This replaces residents elsewhere answering and the faction standing fallback. DevBench op=check lists the owner's people with where the game holds them.
- A press on a locked home door now shows a box: Knock, Pick the lock, Leave. A lock is only picked when chosen. First test: a refused door's second press reached the lock path, Lock Interaction Overhaul's auto-pick took it with its trespass alarm, and the player was jailed; the let-in never ran for that door.
### Fixed
- The prompt shows under Simple Activate, which drops the first line of a door's prompt: while it is loaded the prompt is one line, Knock: name.