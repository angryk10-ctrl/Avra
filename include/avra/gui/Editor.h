#pragma once

#include "avra/gui/Component.h"
#include "avra/core/Processor.h"

namespace avra {

/**
 * @brief Base class for plugin user interface editors.
 */
class Editor : public gui::Component {
public:
    explicit Editor(Processor& processor, int defaultWidth = 550, int defaultHeight = 320)
        : processor_(processor), defaultWidth_(defaultWidth), defaultHeight_(defaultHeight) {
        bounds_ = { 0.0f, 0.0f, static_cast<float>(defaultWidth), static_cast<float>(defaultHeight) };
    }

    [[nodiscard]] Processor& getProcessor() noexcept { return processor_; }
    [[nodiscard]] int getDefaultWidth() const noexcept { return defaultWidth_; }
    [[nodiscard]] int getDefaultHeight() const noexcept { return defaultHeight_; }

protected:
    Processor& processor_;
    int defaultWidth_{550};
    int defaultHeight_{320};
};

} // namespace avra
