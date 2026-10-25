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
#include "synthLib/consoleTools.h"
#include "synthLib/midiTypes.h"

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: n2xOverflashReference /path/to/nord_lead_2x.bin output.wav\n";
        std::cerr << "Reads user-supplied firmware locally; no firmware is included.\n";
        return 2;
    }

    constexpr uint32_t blockSize = 64;
    constexpr uint32_t sampleRate = n2x::g_samplerate; // Native 98304 Hz
    constexpr uint32_t durationSamples = sampleRate * 3;
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
        synthLib::StereoWavSink sink(argv[2], sampleRate);
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
                  << " Hz to " << argv[2] << "\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Nodal Red 2x reference render failed: " << e.what() << "\n";
        return 1;
    }
}
