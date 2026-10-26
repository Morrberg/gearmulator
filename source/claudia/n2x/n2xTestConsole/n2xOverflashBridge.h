#pragma once
// Minimal C ABI used by Overflash. GPL-3.0 as part of Gearmulator.
// Firmware remains user-provided. Native sample rate is 98304 Hz.
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct N2xOverflashHandle N2xOverflashHandle;
// NULL on invalid firmware, initialization error or allocation failure.
N2xOverflashHandle* n2x_overflash_create(const char* rom_path);
void n2x_overflash_destroy(N2xOverflashHandle* handle);
// Returns 98304. No resampling is performed in this bridge.
uint32_t n2x_overflash_sample_rate(void);
// MIDI 0x80/0x90/0xB0 etc. Events apply at the next render block.
int n2x_overflash_midi(N2xOverflashHandle* handle, uint8_t status,
                       uint8_t data1, uint8_t data2);
// Noninterleaved float audio; outputs are clamped to [-1,1].
// The caller owns buffers. Returns 0 on success, negative on error.
int n2x_overflash_render(N2xOverflashHandle* handle, float* left,
                         float* right, uint32_t frames);
#ifdef __cplusplus
}
#endif
