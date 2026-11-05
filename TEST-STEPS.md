# Dynamic Simple Knock System SKSE 1.0.0 - what to try in game

Nothing here has run in game yet. Untick Simpler Knock and its two patches for the test (`SimplerKnock.esp`, `SN_USSEP_Patch.esp`, and the Quiet Start patch if it needs them): its perk text would otherwise override this prompt.

DevBench: `dsk.control` with no op shows the state, `op=check ref=<hex>` gives the rule's verdict and the residents found, `op=box ref=<hex>` opens the box, `op=knock ref=<hex>` is its Knock button. Log: `Documents\My Games\Skyrim Special Edition\SKSE\DynamicSimpleKnock.log`. Set `uLogLevel=1` to see each prompt decision.

## What the first test showed

Log, LIO log and autosave, 2026-11-05: Battle-Born house knocked by day at 06:12:13, let in, 32 seconds inside with 6 residents, no crime, door locked again on leaving. Carlotta's house knocked at night at 06:13:55, refused. The second press went to the game's lock path; LIO's auto-pick took it at 06:13:58 ("Novice lock picked") and raised its trespass alarm; the let-in never ran for that door; he was at a jail cell door by 06:14:30. The prompt was hidden by Simple Activate, which drops the first line of door prompts and reds the name of crime doors.

## Try, in this order

1. **Prompt.** A locked home door shows "Knock: <name>" under Simple Activate.
2. **The box.** Press: Knock, Pick the lock, Leave. Leave does nothing. On keyboard and controller. Check a key-only door has no Pick button.
3. **Day knock.** "Come in.", through the door, no crime. Stand in front of the owner for a few minutes: no complaint and no guards. The Public flag is read by the game in at least three places, but only 32 seconds of play have tested it, so this is the proof wanted.
4. **Night refusal.** `set gamehour to 22`, Knock on a stranger: "It's late...". Press again: the box again, door still locked.
5. **Pick is a choice.** Pick the lock from the box: picking and any crime follow as usual, with LIO's auto-pick if it is on. Expect the crime: this is a real pick.
6. **Friend at night.** `<npc>.setrelationshiprank player 1`, Knock at 22:00: let in.
7. **Leaving.** Out through the door: locked again and private (`events`).
8. **Second cell.** Into a basement while let in: the let-in holds, but that cell is not public, so someone may object (known limit).
9. **Save inside.** Save, reload: `letIn` is filled and the door open. Load an older save: door locked.
10. **Sneak.** Crouch at the door: the door's own prompt and the lock path, no box. If the words lag until you look away, say so.
11. **Who is in there.** The new rule's proof. From outside a door whose owner has a persistent actor (Dragonsreach staff, Carlotta), run `op=check ref=<door>` by day and by night, and read `ownerPeople`: `parent`, `save`, `process`. If `parent` follows their schedule while the cell is unloaded, the rule reads truth. If it stays at their editor cell, it reads the placement. Then walk in and compare with who is really there.
12. **Dead.** `kill` a resident inside, leave, `op=check`: not in `residents`.

## Doors worth trying

Breezehome before buying (should not offer Knock if the key is held), a Whiterun house (faction owner, members placed), Niranye's House in Windhelm (unique owner, not persistent), Wyrmstooth Stonehollow and Kynesgrove (doors Simpler Knock cannot reach), Goldenglow (excluded), a dungeon door (no Knock), Honeyside unbought (no Knock).

## The door rule against the census (before the rule changed to who is in the cell now)

From the survey's plugin scan of 276 locked lived-in doors, with faction reactions added. 265 qualify: 161 faction owner with a member in memory, 3 faction owner with another resident, 74 faction owner with nobody in memory (answer by standing; 56 of those have non-persistent residents the mod cannot see), 20 NPC owner with an actor in memory, 7 unique NPC owner read from the dead count.

The 11 that do not: 4 the player's own, 2 on the exclusion list (Goldenglow, Thalmor Embassy), 5 hostile by the plugin data (Black-Briar Lodge's 4 doors through its mod-added housekeeper's faction, Gautierre Manor through a guard's aggression). In game the actor is asked, so these 5 may differ.

Of the 81 other locked exterior doors: 39 dungeon keyword, 11 exclusion list, 9 the player's own, 4 hostile, 17 not a home, 1 qualifies (the Aretino Residence, no owner, Aventus placed there).
