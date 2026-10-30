# MIDI engine

## Signal path

The plugin is a MIDI generator with silent stereo audio output. Host transport callbacks drive rhythmic scheduling. Notes leave through a per-instance ALSA sequencer client and an Out port; the VST MIDI callback does not provide a second note-output path or MIDI echo.

Each instance owns its kernel scheduling queue, event tags and active-note tracking. There is no engine subprocess, network service or shared preset file.

## Rhythm generation

Each of sixteen lanes stores a 32-bit Euclidean pattern and up to 32 sets of step overrides. For length N, hits K and rotation R, the unrotated step index is mapped circularly and its distributed hit is determined by `(index * K) % N < K`. Manual trigger overrides take precedence.

The engine traverses source steps using FWD, REV, PEND1, PEND2 or a deterministic per-cycle shuffle. Automatic rotation remaps source data to visible positions. Expression remains attached to source steps.

Velocity and probability inherit from the line unless a step override is present. Probability is evaluated once per ratchet group. Humanization modifies the group's starting velocity; ratchet decay scales successive repeats. Zero-velocity repeats produce no note.

## Timing

Step duration in quarter-note beats is 0.125, 0.25, 0.5, 1 or 2. The engine reconstructs each ratchet from its absolute group position. Half-open scheduling intervals prevent repeated boundary hits.

A coverage watermark handles overlapping host position reports without replaying notes. Ordinary timing jitter does not cancel the existing queue. New late events are reanchored with a small lead time; the future queue boundary is not pulled backward.

Note events use absolute ALSA real-time timestamps. Real transport discontinuities invoke the owned-note release path. Host loop boundaries are accounted for separately from seeks.

## Note lifetime

GATE duration is the ratchet subdivision multiplied by the line's gate percentage and converted through the host tempo. Scheduled Note Off is limited by the current host loop boundary.

HOLD sends no duration-based Note Off. The next effective trigger on the same lane schedules the previous Note Off immediately before its Note On at the same timestamp. A held note survives rests and rejected probability decisions. The final ratchet repeat remains held.

Stop, mute, solo exclusion, routing changes and mode changes cancel owned queued events and release owned notes. Failed releases remain pending for retry. Matching note/channel overlaps across lanes are rejected to avoid one lane cancelling another's voice. Distinct note assignments are recommended.

## Resource bounds

- 16 lanes, 32 steps per lane, 8 ratchet hits per step.
- 512 scheduled sub-hits per host callback.
- Up to 64 loop segments per callback.
- Requested ALSA output pool: 4096 events.
- Fixed-size state and notification storage; no file or network I/O in parameter handling.

Exceeding a callback budget invokes the fail-silent release path rather than an unbounded catch-up loop. These bounds are implementation limits, not a throughput guarantee for every tempo and destination.

## State

The PICN4 state stores routing, selection, lane fields, per-step overrides, playback mode, rotation settings, seeds, phase origins and sixteen HOLD flags. Parsing validates the complete payload before replacing engine state. The wrapper reserves 32 KiB for the state chunk. Saving a project does not save an in-flight note or pending phase-reset request as a live event.

## Lane activity feedback

Successful Note On scheduling appends the event time to a visual-only, fixed ring of 64 timestamps per lane. The display consumes due events using the same ALSA queue clock and shows a 140 ms pulse. It never flashes an event early just because the event was scheduled ahead of time.

Numeric display updates retain the approximately 30 Hz cap. Sub-hits closer than 1/120 second can share an indication. Overflow drops visual events only; it does not change MIDI scheduling, consume a note-event budget or increment MIDI errors. Release/reset paths clear pending activity. The indicators are transient and are not serialized into project state.


## Modulation processing

Four slots each own two oscillators and one unique lane assignment. Disabled and inaudible sources suspend phase. Coefficients and static previews are cached. Note On threshold evaluation uses a bounded event budget and common note ownership so stale Note Off events cannot cut a newly owned note. Parameter modulation is applied to playback values without rewriting stored state. CONFIG uses a single effective note per lane; compatible saved states retain the note active in their stored routing mode.

## Native value updates

Engine changes are collected in batches of at most 32 per audio block. Pending host value notifications are coalesced and delivered with a round-robin cursor, at most eight from that queue per block. Pending values remain queued for subsequent blocks. MOD choices update native handles without requesting a global screen rebuild. This limits callback bursts without changing MIDI scheduling.

Applied scale and root metadata are saved with the project. Newly saved states use a SCALE extension that older plugin versions cannot read; retain a project copy before downgrading.


## Extended modulation and cached graphics

The eleven control generators live in `src/mod_lfo.h` and `src/mod_extended.h`. `src/mod_chain.h` applies bounded previous-value cross modulation. `src/mod_preview_worker.h` computes static waveform previews from immutable snapshots on one dedicated worker, without MIDI or host callbacks. Completed results are cached, stale results rejected and requests coalesced to at most ten per second.

The two track pages share a 128-frame miniature atlas with repeated state plateaus. Each passive miniature uses one native control. The MIX and NOTE ON paths remain independent of preview generation.
