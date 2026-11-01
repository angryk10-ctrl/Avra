#include "GainPlugin.h"
#include "GainPluginEditor.h"
#include "avra/format/vst3/Vst3Plugin.h"

AVRA_EXPORT_VST3(
    avra::examples::GainPlugin,
    "com.avra.gain",
    "Avra Gain",
    "Avra Audio",
    "https://github.com/angryk10-ctrl/Avra",
    "1.0.0",
    "Fx|Tools|Stereo"
)
