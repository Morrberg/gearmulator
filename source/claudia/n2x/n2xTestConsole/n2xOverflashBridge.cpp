#include "n2xOverflashBridge.h"
#include "n2xLib/n2xhardware.h"
#include "n2xLib/n2xrom.h"
#include "n2xLib/n2xstate.h"
#include "n2xLib/n2xtypes.h"
#include "dsp56kEmu/utils.h"
#include "synthLib/midiTypes.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

struct N2xOverflashHandle {
    std::unique_ptr<n2x::Hardware> hardware;
    std::unique_ptr<n2x::State> state;
    std::array<float, 64> cachedLeft{};
    std::array<float, 64> cachedRight{};
    uint32_t cachedCursor{64};
};

extern "C" N2xOverflashHandle* n2x_overflash_create(const char* rom_path) {
    if (!rom_path || !rom_path[0]) return nullptr;
    try {
        std::ifstream file(rom_path, std::ios::binary);
        if (!file) return nullptr;
        std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(file),
                                   std::istreambuf_iterator<char>()};
        if (bytes.size() != n2x::g_romSize || !n2x::Rom::isValidRom(bytes))
            return nullptr;
        auto h = std::make_unique<N2xOverflashHandle>();
        h->hardware = std::make_unique<n2x::Hardware>(bytes, rom_path);
        if (!h->hardware->isValid()) return nullptr;
        // Crucial: apply the same default patch initialization as n2x::Device.
        h->state = std::make_unique<n2x::State>(h->hardware.get(), nullptr);
        return h.release();
    } catch (...) { return nullptr; }
}
extern "C" void n2x_overflash_destroy(N2xOverflashHandle* h) {
    delete h;
}
extern "C" uint32_t n2x_overflash_sample_rate(void) {
    return n2x::g_samplerate;
}
extern "C" int n2x_overflash_midi(N2xOverflashHandle* h, uint8_t status,
                                      uint8_t data1, uint8_t data2) {
    if (!h || !h->hardware || status < 0x80 || data1 > 127 || data2 > 127)
        return -1;
    try {
        h->hardware->sendMidi({synthLib::MidiEventSource::Host,
                               status, data1, data2});
        return 0;
    } catch (...) { return -2; }
}
extern "C" int n2x_overflash_render(N2xOverflashHandle* h, float* left,
                                        float* right, uint32_t frames) {
    if (!h || !h->hardware || !left || !right || frames == 0) return -1;
    try {
        // Keep emulation in fixed-size native blocks. Host render requests may
        // be any size. MIDI arriving mid-block is applied on the next native
        // 64-frame boundary (<=63 native frames of MIDI timing quantization).
        auto signedWord = [](dsp56k::TWord value) {
            return dsp56k::signextend<int32_t, 24>(
                static_cast<int32_t>(value));
        };
        for (uint32_t i = 0; i < frames; ++i) {
            if (h->cachedCursor == 64) {
                h->hardware->processAudio(64, 64);
                const auto& outs = h->hardware->getAudioOutputs();
                for (uint32_t j = 0; j < 64; ++j) {
                    const int32_t l = std::clamp(signedWord(outs[0][j])
                        + signedWord(outs[2][j]), -0x800000, 0x7fffff);
                    const int32_t r = std::clamp(signedWord(outs[1][j])
                        + signedWord(outs[3][j]), -0x800000, 0x7fffff);
                    h->cachedLeft[j] = static_cast<float>(l / 8388608.0);
                    h->cachedRight[j] = static_cast<float>(r / 8388608.0);
                }
                h->cachedCursor = 0;
            }
            left[i] = h->cachedLeft[h->cachedCursor];
            right[i] = h->cachedRight[h->cachedCursor];
            ++h->cachedCursor;
        }
        return 0;
    } catch (...) { return -2; }
}
