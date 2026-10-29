# πClideaN16 MPC — User Manual

Version 1.2.5

## 1. Overview

πClideaN is a MIDI instrument: it produces no audio and drives a Drum Track or drum instrument.

This hybrid sequencer combines Euclidean pattern generation, step-by-step editing, parameter locks, selective randomization and probability.

Auto-rotation, different playback directions and LFO modulation let patterns and their triggers evolve.

Each of its sixteen lanes has an independent pattern length and rhythmic division.

Patterns can loop for different durations while staying synchronized to the tempo, creating polymeters and evolving rhythmic offsets.

Five pages: PATTERN, MOD, 01 - 08, 09 - 16 and CONFIG.

**A01–A16** select a pattern (lane).

A white outline identifies the selected pattern.

## 2. Connect a target

Choose the sounds you want to play. πClideaN supplies the rhythm.

Load the plugin on a plugin track, then create a separate Drum Track or drum instrument.

On that target, select this MIDI input:

`piClideaN MPC Out`

Receive **channel 1** or all channels. Each plugin instance has its own port; additional ports are numbered.

Open CONFIG and match each lane’s **MIDI NOTE** to its target sound.

The default notes are **36–51**. All output uses channel 1.

Press Play on the host. πClideaN follows its tempo, position and loops.

Use distinct notes for independent sounds, and avoid receiving the same output through two routes.

## 3. PATTERN — LINE

![PATTERN LINE — A03 selected](images/pattern-line-force-1.2.4.png)

Choose a pattern from **A01 to A16**, then **LINE**. 

Build a rhythm and give the whole lane its character.

### Shape the rhythm

The lower STEPS row sets the pattern:

**STEPS** chooses its length: 1–32 steps.

**HITS** spreads 0–STEPS hits across that length.

**ROTATE** moves the generated pattern around its cycle.

**DIVISION** sets the pace of each step: 1/32, 1/16, 1/8, 1/4 or 1/2 note.

Try different lengths and divisions on different lanes to hear their cycles meet and drift apart.

Manual trigger edits override the generated pattern. Data beyond the current length stays stored.

### Give it expression

The upper LINE row shapes how the lane plays:

**VELOCITY** sets the strength of its hits: 1–127.

**PROBABILITY** lets some hits take a chance: 0–100%, decided once for the whole trigger group, ratchets included.

**HUMANIZE** varies velocity by up to the chosen amount: 0–50, once per group.

**GATE** sets how long notes stay held: 10–95% of a step or ratchet subdivision. It applies in GATE mode; see CONFIG for HOLD.

### Play with the lane

**MUTE** silences the lane.

**SOLO** lets you listen to one or more lanes together. Mute takes precedence.

Active buttons turn **amber**.

**1/2** slows the lane down.

**x2** speeds it up.

Both move through the available rhythmic divisions.

### Selective RANDOM

![RAND — HITS and ROTATE selected on A03](images/rand-selection-force-1.2.4.png)

Keep what you like. Let chance explore the rest.

Press **RAND** once to open selection. The button turns green and italic, with a small white dot.

Tap the control areas you want to vary. They turn **green**; their values stay unchanged for now.

Press **RAND** again to apply the randomization. Only selected parameters change, then selection closes.

All eight LINE controls are available. Here, only HITS and ROTATE are selected.

With nothing selected, nothing changes. Choosing another lane or switching LINE/EDIT cancels selection.

Q-Link editing, 1/2 and x2 pause during selection.

A new Steps value respects unselected Hits and Rotate. If RAND changes Steps, Hits or Rotate, manual trigger overrides are cleared; step expression is retained.

A random result can equal the previous value.

### Rd ALL — sixteen fresh patterns

Tap **Rd ALL**, then **APPLY** in the confirmation panel. **CANCEL** keeps everything unchanged.

Steps, Hits, Rotate and Division are randomized on all sixteen lanes. Step customizations are cleared.

Velocity and Probability return to 100, Humanize to 0 and Gate to 50. Lanes return to GATE; Solo, auto-rotation and MOD reset.

**MUTE states**, **MIDI pitches** and the **applied scale** are preserved.

## 4. PATTERN — EDIT

![EDIT SELECT — A03, step 04, custom velocity 49](images/pattern-edit-force-1.2.4.png)

Give one hit its own character.

Choose **EDIT**, then select a step. The lower **STEP** row now belongs to that step.

**VELOCITY** replaces its lane velocity: 1–127.

**PROBABILITY** replaces its lane probability: 0–100%. The two probabilities are not multiplied.

**RATCHET** turns the step into 1–8 evenly spaced hits.

**DECAY** shapes their velocity: 0–150% of the previous repeat. At 100, repeats stay even; below 100 they fade; above 100 they grow, limited to 127.

At DECAY 0, only the first hit plays. With one hit, Decay has no effect.

Without custom values, velocity and probability follow the lane; ratchet is 1 and decay is 100.

A small dot marks a stored custom value, even if it equals the current lane default. Here, step 04 has velocity 49 and inherits probability 46%.

### Select, draw, erase

**SELECT** explores a step without changing its trigger.

**DRAW** turns the touched trigger on or off.

Changing expression alone does not add a note.

**ERASE** follows your editing mode:

- In SELECT, clear custom step expression and keep the trigger.
- In DRAW, remove the trigger and keep its expression.

Edits are immediate; there is no undo.

The dimmed **LINE** row remains a read-only reference. Return to LINE to change its defaults.

## 5. MOD — dual LFO modulation

![MOD 1 assigned to A16 — Sample & Hold and Note On](images/mod-force-1.2.4.png)

Add movement to a lane, or let a waveform create its own triggers.

Choose **MOD 1–MOD 4**, then assign a **PAD** through the selection panel.

Each lane can use one slot. By default, every slot is **NONE** and both LFOs are **OFF**.

Selecting MOD does not change the lane selected in PATTERN.

### Build the movement

Switch the LFO **ON** and choose a shape:

Sine, Triangle, Ramp Up, Ramp Down, Square, Sample & Hold, Random Smooth or Free Curve.

**SYNC** fits its cycle to the tempo: 1/16 note to 4 bars.

**HZ** sets its frequency independently: 0.05–20 Hz.

**DEPTH** sets the modulation amount: −100 to +100. Negative values invert it.

**OFFSET** shifts its level. **PHASE** shifts its position within the cycle: 0–359°.

For **FREE CURVE**, draw your own shape with eight sliders.

Choose a **DESTINATION**:

NONE, Velocity, Probability, Humanize, Gate, Ratchet, Decay or Note On.

Parameter modulation affects playback. Your stored lane and step values remain intact.

### Mix the two shapes

Turn **MIX** toward the source you want to hear in the modulation:

- −100: LFO 1 only.
- 0: equal blend.
- +100: LFO 2 only.

Each LFO keeps its own destination. An enabled source set to NONE can still contribute to the mix.

### Let the LFO trigger notes

Choose **NOTE ON**, then adjust **THRESHOLD**:

An upward crossing starts a note.

A downward crossing releases it.

While that note is held, step triggers on the same note/channel are suppressed.

The sequencer keeps advancing, then resumes at the next normal trigger after release.

### Read the curves

The previews show **four bars** at the current tempo. They stay still until relevant settings change.

The dashed amber line marks the threshold. Each curve shows one source; actual crossings depend on the mixed signal.

Random shapes show an example, not the next played sequence. Very high frequencies appear as an amplitude band.

OFF and inaudible lanes suspend phase. Re-enabling resumes from that phase without immediately firing a note already above threshold.

Reassigning a PAD releases its previous LFO note.

## 6. Muted lanes and activity

Follow the rhythm by watching **A01–A16**.

An **azure pulse** marks an emitted note reaching playback time. Your selected lane keeps its white outline.

Muted lanes, solo-excluded lanes, rests and probability-rejected hits do not flash. Pulses last about 140 ms; fast ratchets can merge visually.

HOLD flashes on retriggers, not throughout the held note. This feedback represents MIDI activity, not measured sound.

Muted lane buttons are dimmed. Selecting one does not unmute it; its selection border stays visible. Solo exclusion alone does not dim selectors.

### Colors at a glance

**Red step:** trigger on.

**Violet step:** custom expression. A violet rest remains silent.

**White outer border:** selected step.

**Red inner border:** playhead.

**Amber button:** active mode or toggle.

**Green:** RAND selection.

A small dot on a step or expression control marks customization. On a lane button, it marks an enabled modulation destination.

Tap **MODE** to open the playback-direction panel. Select a mode to apply it and close the panel; **CANCEL** keeps the current mode.

## 7. 01 - 08 / 09 - 16 — playback and rotation

![Lanes 01–08 — A07 selected, Auto Rotate enabled](images/tracks-force-1.2.4.png)

Step back and watch eight lanes play together.

Select a lane to work with its controls on the right. **M** and **S** act on their own row.

Each preview shows 32 steps. Muted patterns remain visible; a light outline marks playback without hiding the step’s color.

### Change direction

**FWD / REV:** travel forward or backward.

**PEND1:** go back and forth, repeating endpoints: 1–2–3–4–4–3–2–1.

**PEND2:** turn around without repeating endpoints: 1–2–3–4–3–2.

**URN:** visit every step once, in a shuffled order each cycle.

### Make the pattern rotate

Switch **AUTO ROTATE ON**.

**SHIFT** chooses the movement: −8 to +8 steps.

**EVERY** chooses the interval: 1–16 lane cycles.

A cycle follows the playback mode, not necessarily a host bar. Step expression moves with its trigger; selection follows its stored data.

**RESET PHASE** requests a fresh phase at the next step boundary during playback.

**CYCLE** and **SHIFT** show the current cycle and rotation.

## 8. CONFIG — MIDI notes and GATE / HOLD

![CONFIG — sixteen MIDI notes and GATE / HOLD modes](images/config-force-1.2.4.png)

Give each lane its sound and choose how it holds a note.

**MIDI NOTE** sets the output note: 0–127, with its Force note name, for example **36 · C1**.

Tap the button under **MODE** to switch between GATE and HOLD.

### Play a scale

**SCALES** opens a button panel. Choose DRUM, a scale or a mode.

**START** selects a complete starting note: root and octave.

Changed choices turn green. **APPLY** assigns sixteen ascending notes; the pitches stay unchanged until you apply.

**RESET** restores DRUM, notes 36–51, without changing patterns, step customizations or GATE/HOLD.

If sixteen notes exceed the MIDI range, lower START before applying.

### Short hits: GATE

The dark **GATE** button uses the note duration set in PATTERN.

At 120 BPM and 1/16 division, a step lasts 125 ms. GATE 50 holds it for 62.5 ms; with ratchet 4, each repeat is held for about 15.6 ms.

Host loop boundaries can shorten notes.

### Let it continue: HOLD

The amber **HOLD** button keeps the note held until the lane’s next effective trigger. The old note is released before the new one starts.

Rests and probability-rejected hits leave it playing. Ratchets retrigger; the final repeat remains held.

The Gate percentage stays stored but is inactive in HOLD.

Switching modes, Stop, Mute and Solo exclusion release owned notes.

HOLD suits long samples and sustained instruments. It cannot extend a finished sample: the target must respond to Note Off, which One Shot may ignore.

Use distinct notes for independent lanes. Shared note/channel combinations use overlap protection.

## 9. Q-Links and automation

Reach for the Q-Links to play the controls directly.

On PATTERN in **LINE**:

- Q1–Q4: Velocity, Probability, Humanize, Gate.
- Q5–Q8: Steps, Hits, Rotate, Division.

In **EDIT**:

- Q1–Q4: inactive.
- Q5–Q8: step Velocity, Probability, Ratchet, Decay.

On **MOD**:

- Q1–Q4: LFO 1 Depth, Offset, Phase, Threshold.
- Q5–Q8: the same for LFO 2. MIX has its own knob.

The track pages expose lane selection and playback/rotation controls. CONFIG uses touch and the data wheel.

A white focus outline shows what the wheel will change.

For host automation, use the dedicated lane and step parameters. Contextual aliases depend on selection.

Lane automation still runs in EDIT. Step automation addresses stored source steps, which can move under rotation.

Host automation is separate from MIDI CC input: this plugin does not record or replay CC.

## 10. Save and playback

Found a rhythm you like? **Save the host project.**

It stores patterns, step expression, MIDI notes, GATE/HOLD, playback settings and all four MOD slots.

Held notes and live LFO phases are not saved. Stop releases owned notes.

Probability follows the lane seed and musical position, so a repeating host loop can repeat its decisions.

πClideaN does not create native clips, drive physical pad LEDs or produce audio. Scheduling limits are described in the engine documentation.
