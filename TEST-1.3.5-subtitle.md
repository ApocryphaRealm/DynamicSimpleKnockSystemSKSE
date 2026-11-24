Dynamic Simple Knock System SKSE 1.3.5, second build (commit 81312e3): a knock's answer is subtitled with the resident's name, not "Door".
Report: theanothermoonside on Nexus, 2026-11-24 - with vanilla subtitles, knocking shows the subtitle with the Door as the speaker.
First build failed test 7fd7: the name was taken back after 15 s while the subtitle stayed up for minutes, reading "Door:" again.
This build keeps the name on the door for as long as SubtitleManager holds a subtitle spoken by that door, read every frame;
takes it off before a save and puts it back the next frame while the subtitle is still up; drops it at a load.

Njordlinger Test (SE 1.5.97). Profile: "test Dynamic Simple Knock System SKSE 1.3.5" ticked (reinstall it: the package changed),
every other DSK / Simpler Knock folder off. SkyrimPrefs.ini [Interface] bGeneralSubtitles=1 (restore after).
Log: Documents\My Games\Skyrim Special Edition\SKSE\DynamicSimpleKnock.log. DevBench tool: dsk.control.

1. Startup: "Dynamic Simple Knock System SKSE 1.3.5.0 loading", SE line.
2. Day let-in, Battle-Born outer door 000CB8B9 by day (op=set speech=100 as in 7fd7). Knock with the real keys.
   EXPECTED: op=subtitles during the line shows name "Bergritte Battle-Born" for the door's entry; log
   "door 000CB8B9 named "Bergritte Battle-Born" (the spoken line, the subtitle's speaker)".
3. Keep polling op=subtitles (every second or so) until the door's entry leaves the list (7fd7: about 3 min).
   EXPECTED: every poll while the entry is there shows the resident's name, never "Door"; frames at about 30 s and about 2 min
   show "Bergritte Battle-Born:" on screen; when the entry leaves, the log has "door 000CB8B9 name restored (its subtitle has
   gone): "Door"" and op=subtitles shows "doorNamed":"00000000". Then let-in through the door: works, no bounty.
4. Night refusal, Carlotta Valentia's door 0001A6FC at 22:00 (overrides cleared). Leave the refusal box open for ~40 s, then Leave.
   EXPECTED: the subtitle reads "Carlotta Valentia:" the whole time it is up, before and after the box; the restore comes only
   with "(its subtitle has gone)".
5. Save while the subtitle is up: knock again (op=set resetRefusal=true), save dsk135sub 1-2 s into the line.
   EXPECTED: log "name restored (before a save)" then, a frame later, "named "Carlotta Valentia" (its subtitle is still up)";
   the subtitle keeps reading "Carlotta Valentia:". Load dsk135sub: the door's prompt reads "Knock: Carlotta Valentia's House"
   and GetDisplayName on 0001A6FC is "Door".
6. Close the game; restore SkyrimPrefs.ini; delete the test save.

Send back: op=subtitles JSON at the start, the middle and the end of step 3, the frames, the named/restored log lines in order.
