#pragma once

#include "avra/Avra.h"

namespace avra::examples {

/**
 * @brief Clean example of a stereo Gain & Mute plugin using Avra.
 */
class GainPlugin : public Processor {
public:
    enum ParamIds : ParameterId {
        kGainId = 1,
        kMuteId = 2
    };

    GainPlugin() {
        gainParam_ = &parameters_.add({
            .id = kGainId,
            .name = "Gain",
            .unit = "dB",
            .minValue = -60.0f,
            .maxValue = +12.0f,
            .defaultValue = 0.0f,
            .taper = ParameterTaper::Linear,
            .smoothingTimeMs = 15.0
        });

        muteParam_ = &parameters_.add({
            .id = kMuteId,
            .name = "Mute",
            .minValue = 0.0f,
            .maxValue = 1.0f,
            .defaultValue = 0.0f,
            .taper = ParameterTaper::Stepped,
            .numSteps = 2
        });
    }

    void prepare(double sampleRate, size_t maxBlockSize) override {
        Processor::prepare(sampleRate, maxBlockSize);
    }

    void process(ProcessContext& ctx) override {
        if (ctx.outputs.empty()) return;

        // Copy input to output (handling in-place or distinct buffers)
        if (!ctx.inputs.empty() && ctx.inputs.getChannel(0).data() != ctx.outputs.getChannel(0).data()) {
            ctx.outputs.copyFrom(ctx.inputs);
        }

        const bool isMuted = muteParam_->getPlain() > 0.5f;
        if (isMuted) {
            ctx.outputs.clear();
            peakL_.store(0.0f, std::memory_order_relaxed);
            peakR_.store(0.0f, std::memory_order_relaxed);
            return;
        }

        const size_t numFrames = ctx.numFrames;
        const size_t numChannels = ctx.outputs.numChannels();

        float maxL = 0.0f;
        float maxR = 0.0f;

        // Sample-accurate smoothed gain application
        for (size_t i = 0; i < numFrames; ++i) {
            const float gainDb = gainParam_->getSmoothed();
            const float linearGain = math::dBToLinear(gainDb);

            for (size_t ch = 0; ch < numChannels; ++ch) {
                const float s = (ctx.outputs(ch, i) *= linearGain);
                const float absS = std::abs(s);
                if (ch == 0 && absS > maxL) maxL = absS;
                if (ch == 1 && absS > maxR) maxR = absS;
            }
        }
        if (numChannels == 1) maxR = maxL;
        peakL_.store(maxL, std::memory_order_relaxed);
        peakR_.store(maxR, std::memory_order_relaxed);
    }

    void reset() override {
        peakL_.store(0.0f, std::memory_order_relaxed);
        peakR_.store(0.0f, std::memory_order_relaxed);
    }

    [[nodiscard]] float getPeakL() const noexcept { return peakL_.load(std::memory_order_relaxed); }
    [[nodiscard]] float getPeakR() const noexcept { return peakR_.load(std::memory_order_relaxed); }

    [[nodiscard]] bool hasEditor() const noexcept override { return true; }
    [[nodiscard]] std::unique_ptr<Editor> createEditor() override;

    [[nodiscard]] const char* getName() const noexcept override { return "Avra Gain"; }
    [[nodiscard]] const char* getVendor() const noexcept override { return "Avra Audio"; }

private:
    Parameter* gainParam_{nullptr};
    Parameter* muteParam_{nullptr};
    std::atomic<float> peakL_{0.0f};
    std::atomic<float> peakR_{0.0f};
};

} // namespace avra::examples

#include "GainPluginEditor.h"

namespace avra::examples {
inline std::unique_ptr<Editor> GainPlugin::createEditor() {
    return std::make_unique<GainPluginEditor>(*this);
}
} // namespace avra::examples

