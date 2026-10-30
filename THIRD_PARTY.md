# Third-party notices

## Project code and artwork

Copyright 2026 Timo. MIT License; see LICENSE. The MIDI engine, parameter model and generated artwork are project code.

## MPC VST wrapper and generator

[sd88me/mpc-vst-plugins](https://github.com/sd88me/mpc-vst-plugins), MIT, Copyright 2026 sd88me. Pinned commit: `97a2effca4114dd00f738b018e2f0d1d3ccb2d7f`.

The required wrapper, generator, license, content hashes and local patches are in `third_party/mpc-vst-wrapper/`. Patches cover host timing context, parameter notification lookup, state parsing and chunk storage.

## Native UI schema

The native schema inputs use the sd88me Force Shadow skin conventions. Its license is preserved in `licenses/FORCE-SHADOW-LICENSE`. Generated plugin artwork belongs to this project. MPC and its native controls remain the property of their respective owners.

## Font

Titillium Web Regular is distributed under the SIL Open Font License 1.1. The font and license are in `reference/fonts/`.

## ALSA

ALSA development headers are included as build inputs under their applicable licenses; see `licenses/ALSA-copyright`. The installed plugin links to the target system's `libasound.so.2`; no ALSA runtime is bundled.

## Compiler

The build downloads pinned official Debian compiler packages. Compiler binaries are not included in this repository. See `docs/TOOLCHAIN.json`, `licenses/GCC-copyright` and the notices within downloaded packages.

## Musical references

- [Brugio96/euclidean-sequencer](https://github.com/Brugio96/euclidean-sequencer): Euclidean workflow reference.
- [sd88me/mpc-vst-euclidier](https://github.com/sd88me/mpc-vst-euclidier): sequencer reference.
- [saustin2010/mpc-vst-incipience](https://github.com/saustin2010/mpc-vst-incipience): named drum-instrument note profile.

These applications are not runtime dependencies. No firmware, samples, presets or proprietary instrument binary from these references is included.

## Extended modulation generators

Random Walk, Random Loop, Bounce and cross-LFO control are original MIT implementations. The documented behaviours of [Lunar Modulator](https://github.com/ip2k/lunar-modulator) informed the design; no Lunar audio engine, firmware or source implementation is incorporated.
