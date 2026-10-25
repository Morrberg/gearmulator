// Overflash comparison fixture for Nodal Red 2x.
// Build separately; requires user-installed, compatible Nord Lead 2X firmware.
// GPL-3.0, following the Gearmulator codebase. No ROM assets are included.
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include "n2xLib/n2xhardware.h"
#include "n2xLib/n2xtypes.h"
#include "synthLib/consoleTools.h"
#include "synthLib/midiTypes.h"

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: n2xOverflashReference output.wav\n";
        std::cerr << "Requires a valid Nord Lead 2X ROM in Gearmulator's usual ROM search location.\n";
        return 2;
    }

    constexpr uint32_t blockSize = 64;
    constexpr uint32_t sampleRate = n2x::g_samplerate; // Native 98304 Hz
    constexpr uint32_t durationSamples = sampleRate * 3;
    constexpr uint32_t noteOffSample = sampleRate * 2;
    try
    {
        n2x::Hardware hardware;
        if (!hardware.isValid())
        {
            std::cerr << "Nord Lead 2X ROM missing or invalid\n";
            return 1;
        }
        synthLib::StereoWavSink sink(argv[1], sampleRate);
        std::vector<dsp56k::TWord> left(blockSize), right(blockSize);
        bool noteOnSent = false;
        bool noteOffSent = false;

        for (uint32_t offset = 0; offset < durationSamples; offset += blockSize)
        {
            if (!noteOnSent)
            {
                hardware.sendMidi({synthLib::MidiEventSource::Host, synthLib::M_NOTEON, 60, 102});
                noteOnSent = true;
            }
            if (!noteOffSent && offset >= noteOffSample)
            {
                hardware.sendMidi({synthLib::MidiEventSource::Host, synthLib::M_NOTEOFF, 60, 0});
                noteOffSent = true;
            }

            hardware.processAudio(blockSize, blockSize);
            const auto& outs = hardware.getAudioOutputs();
            for (size_t i = 0; i < blockSize; ++i)
            {
                left[i] = outs[0][i] + outs[2][i];
                right[i] = outs[1][i] + outs[3][i];
            }
            sink.write(left.data(), right.data(), blockSize);
        }
        std::cout << "Rendered " << durationSamples << " frames at " << sampleRate
                  << " Hz to " << argv[1] << "\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Nodal Red 2x reference render failed: " << e.what() << "\n";
        return 1;
    }
}
