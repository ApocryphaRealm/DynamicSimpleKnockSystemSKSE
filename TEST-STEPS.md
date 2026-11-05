# Dynamic Simple Knock System SKSE 1.0.0 - what to try in game

Nothing here has run in game yet. Untick Simpler Knock and its two patches for the test (`SimplerKnock.esp`, `SN_USSEP_Patch.esp`, and the Quiet Start patch if it needs them): its perk text would otherwise override this prompt.

DevBench: `dsk.control` with no op shows the state, `op=check ref=<hex>` gives the rule's verdict and the residents found, `op=knock ref=<hex>` presses on a door. Log: `Documents\My Games\Skyrim Special Edition\SKSE\DynamicSimpleKnock.log`. Set `uLogLevel=1` to see each prompt decision.

## Facts this build needs the game to confirm

Each is the first thing to look at, in this order. The expected result is what the build assumes.

1. **The hooks fire.** Look at a locked home door by day. Expected: "Knock" and the place's name, on keyboard and controller, without any key of ours. `counts.prompts` goes up.
2. **Knock opens.** Press activate by day. Expected: "Come in.", and you go through with no crime and no lockpick menu.
3. **Not trespassing.** Stand in the room with the owner there, wait and walk around. Expected: nobody tells you to leave. This rests on the cell's Public flag, as both older knock mods do. The place in the game that reads the flag was not found by reading, so this is the test that proves it.
4. **It closes behind you.** Leave through the door. Expected: the door is locked again and the cell private (`events` shows it). If the house has a second cell (a basement), walk into it first. Expected: the let-in holds, but that second cell is not public, so someone may object: a known limit.
5. **Save inside.** While let in, save, quit to the menu and load. Expected: the let-in returns (`letIn` is filled) and the door is open. Then load an older save. Expected: the door is locked.
6. **Night.** `set gamehour to 22`, knock at a non-friend. Expected: "It's late...", door locked, the door's own prompt back, and pressing again opens the lock menu. Look away and back: Knock returns.
7. **Friend at night.** `<npc>.setrelationshiprank player 1`, knock at 22:00. Expected: let in.
8. **Sneak.** Crouch while looking at the door. Expected: the prompt changes to the door's own, and back on standing. If the words lag until you look away, the press still does the right thing: report it, the prompt needs one more refresh.
9. **Prompt colour.** A locked owned door may stay red. Report if "Knock" looks like a crime.
10. **Dead.** Kill a persistent resident and the others (console `kill`), then knock on their door. Expected: "Nobody answers." For a unique owner with no actor in memory (`ownerAliveUnseen` true in `op=check`), kill them in their loaded cell, leave, knock: this is the test of the game's dead count.

## Doors worth trying

Breezehome before buying (should not offer Knock if the key is held), a Whiterun house (faction owner, members placed), Niranye's House in Windhelm (unique owner, not persistent), Wyrmstooth Stonehollow and Kynesgrove (doors Simpler Knock cannot reach), Goldenglow (excluded), a dungeon door (no Knock), Honeyside unbought (no Knock).

## The door rule against the census

From the survey's plugin scan of 276 locked lived-in doors, with faction reactions added. 265 qualify: 161 faction owner with a member in memory, 3 faction owner with another resident, 74 faction owner with nobody in memory (answer by standing; 56 of those have non-persistent residents the mod cannot see), 20 NPC owner with an actor in memory, 7 unique NPC owner read from the dead count.

The 11 that do not: 4 the player's own, 2 on the exclusion list (Goldenglow, Thalmor Embassy), 5 hostile by the plugin data (Black-Briar Lodge's 4 doors through its mod-added housekeeper's faction, Gautierre Manor through a guard's aggression). In game the actor is asked, so these 5 may differ.

Of the 81 other locked exterior doors: 39 dungeon keyword, 11 exclusion list, 9 the player's own, 4 hostile, 17 not a home, 1 qualifies (the Aretino Residence, no owner, Aventus placed there).
