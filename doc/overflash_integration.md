# Overflash integration workbench

This branch is reserved for the Overflash ↔ Gearmulator Nord Lead 2X integration.

- Overflash integration branch: https://github.com/Morrberg/Overflash/tree/feature/gearmulator-nordlead2x
- Upstream Gearmulator: https://github.com/dsp56300/gearmulator
- Model under test: Nodal Red 2x (`source/claudia/n2x/n2xLib`)

## Baseline

The fork currently matches upstream Gearmulator's repository tree at the time this branch was created. Existing Gearmulator code remains GPL-3.0; making a fork does not change licensing terms or grant rights over contributors' code.

## Integration design

Keep Gearmulator's original player and DSP execution path intact. First identify a narrow bridge for its `n2x::Device`, including:

1. Device initialization and explicit local firmware input.
2. MIDI note-on/note-off, event timing and MIDI channel handling.
3. Stereo block rendering, sample-rate constraints, and realtime safety.
4. Reset and device-state serialization.
5. Diagnostic logging outside audio callbacks.

An Overflash-side `IInstrumentEngine` adapter will sit behind an explicit opt-in build flag. It must not silently fall back to Overflash's approximate Nord Lead DSP when Gearmulator firmware is missing. An opt-in standalone C++ WAV reference renderer is now committed. It has not yet been compiled or executed with firmware; verify the CMake build and audio output before DPF plugin integration.

## Comparison

Use the Overflash baseline renderer `overflash_nordlead_reference_render` and `tools/compare_nordlead_audio.py` in the Overflash integration branch. Compare matching MIDI phrases at 48 kHz with transparent settings, plus subjective listening. Do not claim circuit-level equivalence based on waveform correlation alone.

## Distribution boundary

**Do not commit original Nord Lead ROMs or firmware, including `nord_lead_2x.bin`.** Keep user assets outside version control and out of artifacts and logs. Preserve GPL and third-party attribution in any distributed combined work.

## Current status

Planning and traceability only. The bridge and functioning Nodal Red 2x audio rendering are **not yet implemented or verified**.


## Offline render commands

On a local checkout of this branch with its required submodules:

```sh
cmake -S . -B build -Dgearmulator_BUILD_JUCEPLUGIN=OFF -DGEARMULATOR_BUILD_OVERFLASH_N2X_REFERENCE=ON
cmake --build build --target n2xOverflashReference --config Release
# Resolve the executable path according to your platform and CMake generator:
./build/source/claudia/n2x/n2xTestConsole/n2xOverflashReference /private/location/nord_lead_2x.bin nodal_reference.wav
```

The executable takes a **local, uncompressed 512 KiB firmware .bin** as its first argument, verifies its Nodal Red 2x signature, and writes the requested WAV to the second argument. Extract the firmware locally from your ZIP; never commit the ZIP or BIN. The test sends MIDI note C4 at time 0, releases it after 2 seconds and captures 3 seconds at the native 98,304 Hz. The model's boot preset is not matched to Overflash's parameters. This is a comparison fixture, not an officially supported standalone release.
