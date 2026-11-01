#pragma once

#include "avra/core/Types.h"
#include "avra/core/AudioSpan.h"
#include "avra/core/ProcessContext.h"
#include "avra/core/ParameterTree.h"

namespace avra {

class Editor;

/**
 * @brief Base class for all Avra audio processors and plugins.
 * 
 * Provides pure virtual lifecycle methods for initialization,
 * real-time block processing, and reset, along with built-in
 * lock-free parameter tree management.
 */
class Processor {
public:
    virtual ~Processor() = default;

    /**
     * @brief Called when audio device starts or sample rate / buffer size changes.
     * All DSP allocations and buffer resizes should be done here, NEVER in process().
     */
    virtual void prepare(double sampleRate, size_t maxBlockSize) {
        sampleRate_ = sampleRate;
        maxBlockSize_ = maxBlockSize;
        parameters_.prepare(sampleRate);
    }

    /**
     * @brief Real-time audio rendering loop.
     * GUARANTEE: Never allocates heap memory, takes locks, or performs I/O.
     */
    virtual void process(ProcessContext& context) = 0;

    /**
     * @brief Reset internal DSP histories (delay lines, filter states, envelopes).
     */
    virtual void reset() = 0;

    /// GUI Editor support
    [[nodiscard]] virtual bool hasEditor() const noexcept { return false; }
    [[nodiscard]] virtual std::unique_ptr<Editor> createEditor() { return nullptr; }

    /// Access parameter tree
    [[nodiscard]] ParameterTree& getParameters() noexcept { return parameters_; }
    [[nodiscard]] const ParameterTree& getParameters() const noexcept { return parameters_; }


    [[nodiscard]] double getSampleRate() const noexcept { return sampleRate_; }
    [[nodiscard]] size_t getMaxBlockSize() const noexcept { return maxBlockSize_; }
    [[nodiscard]] virtual uint32_t getLatencySamples() const noexcept { return 0; }

    /// Plugin Identification
    [[nodiscard]] virtual const char* getName() const noexcept = 0;
    [[nodiscard]] virtual const char* getVendor() const noexcept = 0;
    [[nodiscard]] virtual const char* getVersion() const noexcept { return "1.0.0"; }

    /// Factory Program / Preset Support (for VST3 IUnitInfo & Top-Bar Selector)
    [[nodiscard]] virtual int getProgramCount() const noexcept { return 6; }
    [[nodiscard]] virtual const char* getProgramName(int index) const noexcept {
        static constexpr const char* kDefaultNames[6] = {
            "Init / Flat", "Drums", "Bass", "Guitar", "Keys / Synth", "Vocals"
        };
        if (index < 0 || index >= 6) return "Init / Flat";
        return kDefaultNames[index];
    }
    [[nodiscard]] virtual int getCurrentProgram() const noexcept { return currentProgram_; }
    virtual void setCurrentProgram(int index) noexcept { currentProgram_ = index; }

protected:
    ParameterTree parameters_;
    double sampleRate_{48000.0};
    size_t maxBlockSize_{512};
    int currentProgram_{0};
};

} // namespace avra
