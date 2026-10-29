# Installation and removal (Gen2, aarch64)

This guide ships inside `piClideaN-MPC-1.2.4-aarch64.zip`. For Gen1 and Force, use `piClideaN-MPC-1.2.4-armv7.zip` and its own guide.

## Requirements

- Akai MPC Gen2 (MPC Live III and kin): a 64-bit aarch64 userland. Check with `uname -m`, which must print `aarch64`.
- Root shell access (SSH) or a compatible installer that handles aarch64 portable plugin ZIPs and the `pluginList-arm-64bit` registration.
- A writable, executable `Synths` folder. Removable drives are often mounted `noexec` and cannot load a plugin. On an MPC Live III the internal `/nvme/Synths` is writable and executable; `/synths` is read-only.

Tested on an MPC Live III (aarch64, az0x base 5.0.17). Other Gen2 models are untested.

## Install by hand

1. Save the current project, then stop playback.
2. Unzip `piClideaN-MPC-1.2.4-aarch64.zip`. The plugin folder is `portable/Timo - VST - πClideaN MPC`.
3. Copy that folder to the device's `Synths` folder, for example `/nvme/Synths/Timo - VST - πClideaN MPC`. Keep the folder name and the `.so` executable.
4. Back up `MPC.settings` (on a Live III: `/data/Settings/MPC/MPC.settings`). Stop MPC (`systemctl stop acvs`) before editing it. MPC rewrites the file while it runs.
5. In `MPC.settings`, add this entry as the first child of `<VALUE name="pluginList-arm-64bit"><KNOWNPLUGINS>`, adjusting the path to your `Synths` folder:

```xml
<PLUGIN name="πClideaN MPC" descriptiveName="πClideaN MPC" format="VST"
        category="Synth" manufacturer="Timo" version="1.2.4"
        file="/nvme/Synths/Timo - VST - πClideaN MPC/piclidean_mpc.so"
        uid="5069434e" isInstrument="1" fileTime="0" infoUpdateTime="0"
        numInputs="0" numOutputs="2" isShell="0"/>
```

6. Start MPC again (`systemctl start acvs`).
7. Create a Plugin Track and select **πClideaN MPC**.
8. Create or select the target Drum Track or drum instrument, with samples on the pads.
9. On the target track, enable `piClideaN MPC Out` as its MIDI input and receive channel 1 or all channels.
10. Start the transport.

The content directory is `Timo - VST - πClideaN MPC`. Do not rename the binary, content directory or skin identifier independently. They form a single plugin identity.

## Remove

1. Stop playback and save any project changes you want to keep.
2. Remove instances from projects, and the MIDI input assignments from target tracks.
3. Stop MPC, remove the `PLUGIN` entry above from `MPC.settings` (or restore your backup), delete the `Timo - VST - πClideaN MPC` folder, and start MPC.

Project files and samples are separate from the plugin package.

## Routing checklist

- πClideaN produces MIDI, not audio: its own audio output is silent.
- Enable its output on the intended target track, using the same MIDI channel.
- Match the MIDI note in CONFIG to the target pad or sound.
- Disable unwanted parallel MIDI inputs to avoid duplicate hits.
- For multiple instances, choose the corresponding numbered output port.
- In HOLD, the target must respond to Note Off; One Shot playback may ignore it.

## Gen2 skin note

MPC Live III draws a non-button `Image` component as a blank box when its bitmap is smaller than about 5400 pixels in area. The Gen2 package therefore ships a skin whose small images are padded to at least 6400 pixels (MPC clips them to their bounds). This is done at build time with `PICN_SKIN_MIN_IMAGE_AREA`; the Gen1 skin is unchanged.
