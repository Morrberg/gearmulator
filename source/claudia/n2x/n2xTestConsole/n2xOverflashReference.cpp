// Overflash comparison fixture for Nodal Red 2x.
// Build separately; requires user-installed, compatible Nord Lead 2X firmware.
// GPL-3.0, following the Gearmulator codebase. No ROM assets are included.
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include "n2xLib/n2xrom.h"
#include <iostream>
#include <memory>
#include <vector>
#include "n2xLib/n2xhardware.h"
#include "n2xLib/n2xtypes.h"
#include "n2xLib/n2xstate.h"
#include "dsp56kEmu/utils.h"
#include "synthLib/consoleTools.h"
#include "synthLib/midiTypes.h"

int main(int argc, char** argv)
{
    if (argc < 3 || argc > 4)
    {
        std::cerr << "Usage: n2xOverflashReference /path/to/nord_lead_2x.bin output.wav [diagnostic|raw|matched]\n";
        std::cerr << "Reads user-supplied firmware locally; no firmware is included.\n";
        return 2;
    }

    // Diagnostic mode compares note-off and an additional All Notes Off
    // command. The latter is deliberately absent from the standard A/B case.
    const bool diagnostic = argc == 4 && std::string(argv[3]) == "diagnostic";
    const bool raw = argc == 4 && std::string(argv[3]) == "raw";
    const bool matched = argc == 4 && std::string(argv[3]) == "matched";
    if (argc == 4 && !diagnostic && !raw && !matched) {
        std::cerr << "Unknown mode; expected diagnostic, raw or matched\n";
        return 2;
    }
    constexpr uint32_t blockSize = 64;
    constexpr uint32_t sampleRate = n2x::g_samplerate; // Native 98304 Hz
    constexpr uint32_t durationSamples = sampleRate * 4;
    constexpr uint32_t noteOffSample = sampleRate * 2;
    try
    {
        std::ifstream input(argv[1], std::ios::binary);
        if (!input) { std::cerr << "Could not open firmware file\n"; return 1; }
        const std::vector<uint8_t> romBytes{std::istreambuf_iterator<char>(input),
                                            std::istreambuf_iterator<char>()};
        if (romBytes.size() != n2x::g_romSize || !n2x::Rom::isValidRom(romBytes))
        {
            std::cerr << "Firmware fails Nord Lead 2X size/signature checks\n";
            return 1;
        }
        n2x::Hardware hardware(romBytes, argv[1]);
        if (!hardware.isValid())
        {
            std::cerr << "Nord Lead 2X ROM missing or invalid\n";
            return 1;
        }
        // The regular n2x::Device constructs State immediately after Hardware.
        // State supplies a standard single/multi patch and queues it as SysEx.
        // 'raw' intentionally reproduces the historical test's missing init.
        std::unique_ptr<n2x::State> patchState;
        if (!raw)
        {
            patchState = std::make_unique<n2x::State>(&hardware, nullptr);
            if (matched)
            {
                // Controlled single oscillator test. Register values are N2X
                // device controls, NOT exact Hz/seconds conversions.
                auto preset = patchState->getSingle(0);
                const auto set = [&](n2x::SingleParam key, uint8_t value) {
                    n2x::State::changeSingleParameter(preset, key, value);
                };
                set(n2x::SingleParam::O1Waveform, 0); // saw on N2X
                set(n2x::SingleParam::Mix, 0);        // oscillator 1 only
                set(n2x::SingleParam::Cutoff, 127);   // maximum
                set(n2x::SingleParam::Resonance, 0);
                set(n2x::SingleParam::FilterEnvAmount, 0);
                set(n2x::SingleParam::FilterType, 0);
                set(n2x::SingleParam::AmpEnvA, 0);
                set(n2x::SingleParam::AmpEnvD, 0);
                set(n2x::SingleParam::AmpEnvS, 127);
                set(n2x::SingleParam::AmpEnvR, 0);
                set(n2x::SingleParam::Lfo1Level, 0);
                set(n2x::SingleParam::FmDepth, 0);
                set(n2x::SingleParam::Unison, 0);
                set(n2x::SingleParam::Portamento, 0);
                synthLib::SMidiEvent patch(synthLib::MidiEventSource::Host);
                patch.sysex.assign(preset.begin(), preset.begin() + n2x::g_singleDumpSize);
                hardware.sendMidi(patch);
                std::cerr << "Matched patch queued: saw 1, osc 2 off, "
                          << "max cutoff, no resonance/modulation, "
                          << "instant attack/release and full sustain\\n";
            }
            std::cerr << "Queued Gearmulator default single/multi patch\\n";
        }
        else
            std::cerr << "Raw hardware mode: no patch initialization\n";

        synthLib::StereoWavSink sink(argv[2], sampleRate);
        std::vector<dsp56k::TWord> left(blockSize), right(blockSize);
        bool noteOnSent = false;
        bool noteOffSent = false;
        bool allNotesOffSent = false;

        for (uint32_t offset = 0; offset < durationSamples; offset += blockSize)
        {
            if (!noteOnSent)
            {
                hardware.sendMidi({synthLib::MidiEventSource::Host, synthLib::M_NOTEON, 60, 102});
                noteOnSent = true;
                std::cerr << "MIDI Note On: frame " << offset << " (C4, velocity 102)\n";
            }
            if (!noteOffSent && offset >= noteOffSample)
            {
                hardware.sendMidi({synthLib::MidiEventSource::Host, synthLib::M_NOTEOFF, 60, 0});
                noteOffSent = true;
                std::cerr << "MIDI Note Off: frame " << offset << " (C4)\n";
            }

            if (diagnostic && !allNotesOffSent && offset >= sampleRate * 3)
            {
                hardware.sendMidi({synthLib::MidiEventSource::Host,
                                   synthLib::M_CONTROLCHANGE, 123, 0});
                allNotesOffSent = true;
                std::cerr << "MIDI CC123 All Notes Off: frame " << offset << "\n";
            }
            hardware.processAudio(blockSize, blockSize);
            const auto& outs = hardware.getAudioOutputs();
            for (size_t i = 0; i < blockSize; ++i)
            {
                // Hardware exposes signed 24-bit DSP words encoded in TWord.
                // Saturate in signed 32-bit space before encoding back to 24-bit.
                const auto combine = [](dsp56k::TWord a, dsp56k::TWord b) {
                    const int32_t x = dsp56k::signextend<int32_t, 24>(static_cast<int32_t>(a));
                    const int32_t y = dsp56k::signextend<int32_t, 24>(static_cast<int32_t>(b));
                    return static_cast<dsp56k::TWord>(
                        std::clamp(x + y, -0x800000, 0x7fffff)) & 0xffffff;
                };
                left[i] = combine(outs[0][i], outs[2][i]);
                right[i] = combine(outs[1][i], outs[3][i]);
            }
            sink.write(left.data(), right.data(), blockSize);
        }
        std::cout << "Rendered " << durationSamples << " frames at " << sampleRate
                  << " Hz to " << argv[2] << "\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Nodal Red 2x reference render failed: " << e.what() << "\n";
        return 1;
    }
}
