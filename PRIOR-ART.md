# Prior art for Dynamic Simple Knock System SKSE (searched 2026-11-05, rule 24)

Wanted: knock on the locked door of somebody's home, worked out at run time from who owns the door or the space behind it, with no plugin and no patches, on SE 1.5.97, on keyboard and controller.

Searched Nexus (through its API, by name): knock, knock door, knocking, locked door, ask to enter, let me in and related terms, plus a general web search. Full write-up, with a census of the 276 locked lived-in doors in the test list: `D:\Claude output\4. plans\list-updates-2026-11-04\knock-mod-survey.md`.

Searched GitHub (`gh search repos`, 2026-11-05): "skyrim knock door", "skse knock", "simply knock", "skyrim locked door owner activate skse". Hits: chesko256/SimplyKnock, SilentStorm064/SimplyKnock-for-GOG-1.6.1179, eXgamble/MKF-Simply-Knock.

| Mod | Where | How it decides | Why it is not this |
|---|---|---|---|
| Simpler Knock 3.1 | Nexus 31585 | Perk and script. Needs a key record and a placed Boss-tagged actor carrying that key. | 151 of 276 doors by its rule. Needs a data patch per mod. One failed knock can exclude a whole door model for the rest of a save. |
| Simply Knock SE 1.0.3 | Nexus 14098, github chesko256/SimplyKnock | Script with an SKSE helper. Door has a linked door and a key the player lacks, destination location is House, Dwelling or Farm. | The nearest idea. 248 of 276 by its rule as calculated in the survey. Needs a key on the door, a pop-up menu, more dependencies; no time of day. |
| Just Knock 1.1 | Nexus 103004 | Conditions on Simply Knock, with Dynamic Activation Key | Inherits Simply Knock |
| Simpler Knock Tweaks and Fixes 1.1 | Nexus 170673 | Perk and text edits | No coverage change |
| Simply Knock Animated, Tower Stone Fix, Wintersun and Andromeda patches, Knock Sound Replacer | Nexus 121994, 41590, 105284 and others | Add-ons | No coverage change |

No mod found reads ownership instead of keys, or answers by the hour and the relationship rank. Nothing is copied from any of them: both prior knock mods set the destination cell public on success, which is the idea taken here, and the code is written separately.
