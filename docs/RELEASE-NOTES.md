# Version 1.3.0

- Add Random Walk, Random Loop and Bounce to the eleven-shape MOD palette.
- Add pulse width, random step size, loop evolution, gravity and restitution controls.
- Chain either LFO into the other’s rate, depth or offset. Preserve the existing NOTE ON and threshold behavior.
- Cache static waveform previews and compute them on a dedicated background worker.
- Reduce queued display notification bursts and remove unused preview notifications.
- Share identical image assets and streamline the miniature steps on both track pages.
- Correct native miniature frame slicing and retain note, custom-value and playhead colours.
- Update the illustrated user manual.

Musical operation and interface improvements were validated by the user on Akai Force. Crackles previously reported during interaction were no longer observed in the tested setup. CPU and effects loads vary by project; no universal performance guarantee is made.

Projects save the PICN7 state format; PICN6 states can be loaded. Earlier plugin versions cannot load the new state format. Parameter indices remain unchanged.
