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

An Overflash-side `IInstrumentEngine` adapter will sit behind an explicit opt-in build flag. It must not silently fall back to Overflash's approximate Nord Lead DSP when Gearmulator firmware is missing. The first milestone is a standalone, reproducible WAV reference render before DPF plugin integration.

## Comparison

Use the Overflash baseline renderer `overflash_nordlead_reference_render` and `tools/compare_nordlead_audio.py` in the Overflash integration branch. Compare matching MIDI phrases at 48 kHz with transparent settings, plus subjective listening. Do not claim circuit-level equivalence based on waveform correlation alone.

## Distribution boundary

**Do not commit original Nord Lead ROMs or firmware, including `nord_lead_2x.bin`.** Keep user assets outside version control and out of artifacts and logs. Preserve GPL and third-party attribution in any distributed combined work.

## Current status

Planning and traceability only. The bridge and functioning Nodal Red 2x audio rendering are **not yet implemented or verified**.
