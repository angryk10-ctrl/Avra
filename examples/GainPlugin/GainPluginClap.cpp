#include "GainPlugin.h"
#include "avra/format/clap/ClapPlugin.h"

AVRA_EXPORT_CLAP(
    avra::examples::GainPlugin,
    "com.avra.gain",
    "Avra Gain",
    "Avra Audio",
    "https://github.com/angryk10-ctrl/Avra",
    "1.0.0",
    "Clean, transparent stereo gain and mute plugin built with Avra",
    CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
    CLAP_PLUGIN_FEATURE_UTILITY
)
