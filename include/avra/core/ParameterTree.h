#pragma once

#include "avra/core/Parameter.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <sstream>

namespace avra {

/**
 * @brief Container managing all plugin parameters, indexing, and state serialization.
 */
class ParameterTree {
public:
    ParameterTree() = default;

    /// Add a new parameter configuration to the tree
    Parameter& add(Parameter::Config config) {
        auto param = std::make_unique<Parameter>(std::move(config));
        Parameter* ptr = param.get();
        idMap_[ptr->getId()] = ptr;
        parameters_.push_back(std::move(param));
        return *ptr;
    }

    /// Prepare all parameter smoothers for playback
    void prepare(double sampleRate) noexcept {
        for (auto& param : parameters_) {
            param->prepare(sampleRate);
        }
    }

    /// Get total number of parameters
    [[nodiscard]] size_t size() const noexcept {
        return parameters_.size();
    }

    /// Access parameter by index
    [[nodiscard]] Parameter* getByIndex(size_t index) noexcept {
        if (index < parameters_.size()) {
            return parameters_[index].get();
        }
        return nullptr;
    }

    [[nodiscard]] const Parameter* getByIndex(size_t index) const noexcept {
        if (index < parameters_.size()) {
            return parameters_[index].get();
        }
        return nullptr;
    }

    /// Access parameter by unique ID (O(1))
    [[nodiscard]] Parameter* getById(ParameterId id) noexcept {
        auto it = idMap_.find(id);
        return (it != idMap_.end()) ? it->second : nullptr;
    }

    [[nodiscard]] const Parameter* getById(ParameterId id) const noexcept {
        auto it = idMap_.find(id);
        return (it != idMap_.end()) ? it->second : nullptr;
    }

    /// Fast array indexing operator
    [[nodiscard]] Parameter& operator[](size_t index) noexcept {
        return *parameters_[index];
    }

    [[nodiscard]] const Parameter& operator[](size_t index) const noexcept {
        return *parameters_[index];
    }

    /// Serialize all parameter states to a compact string format (for DAW project save)
    [[nodiscard]] std::string serializeState() const {
        std::ostringstream ss;
        ss << "AVRA_STATE_V1\n";
        ss << parameters_.size() << "\n";
        for (const auto& param : parameters_) {
            ss << param->getId() << " " << param->getNormalized() << "\n";
        }
        return ss.str();
    }

    /// Restore parameter states from a saved state string
    bool restoreState(std::string_view stateStr) {
        std::istringstream ss{std::string(stateStr)};
        std::string header;
        if (!(ss >> header) || header != "AVRA_STATE_V1") {
            return false;
        }

        size_t count = 0;
        if (!(ss >> count)) return false;

        for (size_t i = 0; i < count; ++i) {
            ParameterId id = 0;
            float norm = 0.0f;
            if (ss >> id >> norm) {
                if (auto* param = getById(id)) {
                    param->setNormalized(norm);
                }
            }
        }
        return true;
    }

private:
    std::vector<std::unique_ptr<Parameter>> parameters_;
    std::unordered_map<ParameterId, Parameter*> idMap_;
};

} // namespace avra
