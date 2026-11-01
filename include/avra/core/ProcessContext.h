#pragma once

#include "avra/core/Types.h"
#include "avra/core/AudioSpan.h"
#include "avra/core/MidiEvent.h"

namespace avra {

/**
 * @brief Host transport playback state.
 */
struct TransportInfo {
    double bpm{120.0};
    int32_t timeSigNumerator{4};
    int32_t timeSigDenominator{4};
    int64_t songPositionSamples{0};
    double songPositionBeats{0.0};
    bool isPlaying{false};
    bool isRecording{false};
    bool isLooping{false};
};

/**
 * @brief Context passed to Processor::process() on every audio block.
 */
struct ProcessContext {
    AudioSpan<const Sample> inputs;
    AudioSpan<Sample> outputs;
    MidiSpan midiInputs;
    TransportInfo transport;
    double sampleRate{48000.0};
    size_t numFrames{0};

    [[nodiscard]] constexpr size_t numInputChannels() const noexcept {
        return inputs.numChannels();
    }

    [[nodiscard]] constexpr size_t numOutputChannels() const noexcept {
        return outputs.numChannels();
    }
};

} // namespace avra
