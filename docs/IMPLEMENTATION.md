# Implementation

## Modules

- `src/engine.c`: instance lifecycle, parameter access, ALSA scheduling, note ownership and host transport.
- `src/evolution.h`: rhythmic traversal, step overrides, Q-Link aliases and state serialization.
- `src/mod_lfo.h`: four dual-LFO slots, static preview cache, modulation and threshold scheduling.
- `src/mod_types.h`: bounded modulation state and parameter declarations.
- `src/drum_notes.h`: generated 128-entry note-map table.
- `scripts/generate.py`: parameter definitions, VST configuration and note-map generation.
- `scripts/build_skin.py`: native UI artwork and component definitions.
- `scripts/build.py`: pinned-wrapper checks and ARMv7 compilation.
- `scripts/package.py`: portable installation ZIP assembly.

## Identity and routing

Display name: πClideaN MPC. Project name: πClideaN16 MPC. Technical name: piClideaN MPC. VST UID: PiCN / 5069434e. Binary: piclidean_mpc.so. Content folder: Timo - VST - πClideaN MPC.

An instance acquires a bounded unique slot. Its ALSA client, Out port and kernel queue are independent of other instances. Slot release and owned-note cleanup occur during destruction.

## Parameter model

The model contains 3974 fixed indices. `params.lock` defines the public index order. New parameters must be appended; rebuilding fails if existing indices are changed. Some indices are reserved and remain in the model without visible controls.

Canonical lane and step parameters have stable destinations for automation. Eight contextual Q-Link aliases resolve to the currently selected lane and step. Fractional Q-Link movement is retained when mapping to narrow integer ranges.

Parameter writes do not request a native screen rebuild by default. Explicit page/dialog selectors request the needed display refresh. Numeric notifications are coalesced in fixed storage; a sorted lookup resolves parameter names. Dynamic step and playback visuals are updated through bounded notifications.

## Native UI

The plugin supplies five 1280 × 628 content pages; MPC supplies the surrounding header and tabs. PATTERN provides two rows of lane selectors, eight contextual controls and two rows of sixteen steps. The two track pages cover lanes 01–08 and 09–16; CONFIG lists all sixteen lanes on one page.

A step's native touch owner contains its passive state artwork. Native toggle controls own persistent buttons such as mute, solo and HOLD. Artwork does not create a separate interactive layer above these controls. Focus is white; triggers are red, custom steps violet and active buttons muted amber.

The layout and bitmap sizes are generated together. Native controls bind to fixed parameter indices. Setup dialogs belong to their SET page and obscure underlying controls while open.

## Dependencies

The vendored MPC VST wrapper and parameter generator are pinned by commit and content hashes. Local wrapper patches provide host context, bounded notification lookup, state validation and state capacity. ALSA headers, the native skin schema and Titillium Web regular font are provided as build inputs. System audio/MIDI libraries remain dynamically linked.

The source tree contains no firmware, sample library or proprietary instrument engine. See THIRD_PARTY.md for license attribution.

## Azure indicators

Sixteen read-only lane_flash parameters publish playback activity. They are excluded from canonical automation and do not alter the PICN4 state. Each PATTERN lane selector retains its native selection binding, with normal and azure artwork variants selected by its activity indicator. The selected variant retains the white outline. There is no separate passive hit surface above the selection control.

## Selective randomization

RAND uses transient selection state and an eight-bit parameter mask. Seventeen appended non-automatable UI parameters expose mode, target indicators and touch actions. No new MIDI output, queue or scheduler is introduced. Selection is not serialized in PICN4.

## LINE references in EDIT

EDIT renders the four LINE controls with dimmed artwork, no Q-Link action and no hardware focus. Contextual q0–q3 writes and selected-lane Velocity, Probability, Humanize and Gate aliases are rejected while EDIT is active. Canonical lN_* parameters remain writable for host automation. The four selected-step controls remain active. Returning to LINE restores normal editing. Parameter indices and PICN4 state are unchanged.

## Muted lane selectors

Each A01–A16 selector uses mutually exclusive native button states for normal idle, normal activity and explicit mute. Mute takes priority over activity artwork. Selection keeps its white outline. The states reuse existing lN_mute and lane_flashN parameters; Solo exclusion is not treated as explicit mute. No separate passive hit layer, extra scheduler or MIDI output is introduced.

## Rounded skin artwork

A shared 10-pixel radius in native 1280 × 628 coordinates is used for control artwork, with proportionate radii for the main frame and miniature step tiles. Rounded backgrounds include their own fill; selected outlines match their control. Passive transparent PNG overlays implement rounded native-focus feedback with the existing WhenFocussed visibility condition. They introduce no touch actions. Small state dots retain their original enabling conditions and use an inset position. Native MPC header and tab rendering remain host-owned.


## Scoped display notifications

PATTERN lane selection does not invalidate MOD visual caches. MOD context changes retain their own notification path. CONFIG headings use native Name labels backed by existing read-only handles; their indices are unchanged.

## Native value updates

Engine changes are collected in batches of at most 32 per audio block. Pending host value notifications are coalesced and delivered with a round-robin cursor, at most eight from that queue per block. Pending values remain queued for subsequent blocks. MOD choices update native handles without requesting a global screen rebuild. This limits callback bursts without changing MIDI scheduling.

Applied scale and root metadata are saved with the project. Newly saved states use a SCALE extension that older plugin versions cannot read; retain a project copy before downgrading.


## Extended modulation and cached graphics

The eleven control generators live in `src/mod_lfo.h` and `src/mod_extended.h`. `src/mod_chain.h` applies bounded previous-value cross modulation. `src/mod_preview_worker.h` computes static waveform previews from immutable snapshots on one dedicated worker, without MIDI or host callbacks. Completed results are cached, stale results rejected and requests coalesced to at most ten per second.

The two track pages share a 128-frame miniature atlas with repeated state plateaus. Each passive miniature uses one native control. The MIX and NOTE ON paths remain independent of preview generation.
