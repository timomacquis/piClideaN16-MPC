# Installation and removal

This guide is for Gen1 and Force. MPC Gen2 (Live III and kin, aarch64) needs `piClideaN-MPC-1.2.4-aarch64.zip` and [Installation (Gen2)](INSTALLATION-GEN2.md).

## Requirements

- Akai MPC Gen1 with community ARMv7 VST2 plugin support and ALSA MIDI routing. (Tested on FORCE)
- A compatible installer that handles portable plugin ZIPs and MPC plugin registration.
- For SSD storage, an executable `Synths` mount, configured separately from this plugin.

The package contains regular files, requires no symbolic links and can be stored on exFAT. Filesystem support does not replace the executable-mount requirement.

## Install

1. Save the current project before allowing an installer to restart MPC.
2. Open the installer's local ZIP installation page.
3. Select `piClideaN-MPC-1.2.4-armv7.zip` from `Installation/`.
4. Select **User memory / Synths** or the available **SSD / Synths** destination.
5. Review the destination and complete the installation. Allow the requested MPC restart after saving.
6. Create a Plugin Track and select **πClideaN MPC**.
7. Create or select the target Drum Track or drum instrument.
8. On the target track, enable `piClideaN MPC Out` as its MIDI input and receive channel 1 or all channels.
9. Start the Force transport.

The content directory is `Timo - VST - πClideaN MPC`. Typical locations are:

```text
/sdcard/Synths/Timo - VST - πClideaN MPC/
/media/<drive>/Synths/Timo - VST - πClideaN MPC/
```

Do not rename the binary, content directory or skin identifier independently. They form a single plugin identity.

## Remove

1. Stop playback and save any project changes you want to keep.
2. Remove instances from projects you intend to use without this plugin, and remove their MIDI input assignments from target tracks as needed.
3. In the compatible installer's installed-plugin management page, select **πClideaN MPC** and choose its removal action.
4. Review the selected plugin and location. Complete removal and allow the requested MPC restart after saving.

Use the installer's removal workflow so the plugin files and its registration are handled together. Deleting a folder alone leaves its registration behind. Project files and samples are separate from the plugin package.

## Routing checklist

- πClideaN produces MIDI, not audio: its own audio output is silent.
- Enable its output on the intended target track, using the same MIDI channel.
- Match the MIDI note in CONFIG to the target pad or sound.
- Disable unwanted parallel MIDI inputs to avoid duplicate hits.
- For multiple instances, choose the corresponding numbered output port.
- In HOLD, the target must respond to Note Off; One Shot playback may ignore it.
