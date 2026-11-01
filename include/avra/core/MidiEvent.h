#pragma once

#include "avra/core/Types.h"
#include <span>

namespace avra {

/**
 * @brief Sample-accurate MIDI 1.0 event.
 */
struct MidiEvent {
    uint32_t sampleOffset{0}; // Sample index within the current audio block
    uint8_t status{0};
    uint8_t data1{0};
    uint8_t data2{0};

    [[nodiscard]] constexpr uint8_t getChannel() const noexcept {
        return status & 0x0F;
    }

    [[nodiscard]] constexpr uint8_t getCommand() const noexcept {
        return status & 0xF0;
    }

    [[nodiscard]] constexpr bool isNoteOn() const noexcept {
        return getCommand() == 0x90 && data2 > 0;
    }

    [[nodiscard]] constexpr bool isNoteOff() const noexcept {
        return getCommand() == 0x80 || (getCommand() == 0x90 && data2 == 0);
    }

    [[nodiscard]] constexpr bool isControlChange() const noexcept {
        return getCommand() == 0xB0;
    }

    [[nodiscard]] constexpr bool isPitchBend() const noexcept {
        return getCommand() == 0xE0;
    }

    [[nodiscard]] constexpr uint8_t getNoteNumber() const noexcept {
        return data1;
    }

    [[nodiscard]] constexpr uint8_t getVelocity() const noexcept {
        return data2;
    }

    [[nodiscard]] constexpr uint8_t getControllerNumber() const noexcept {
        return data1;
    }

    [[nodiscard]] constexpr uint8_t getControllerValue() const noexcept {
        return data2;
    }
};

using MidiSpan = std::span<const MidiEvent>;

} // namespace avra
