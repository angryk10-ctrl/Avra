#pragma once

/**
 *  █████╗ ██╗   ██╗██████╗  █████╗ 
 * ██╔══██╗██║   ██║██╔══██╗██╔══██╗
 * ███████║██║   ██║██████╔╝███████║
 * ██╔══██║╚██╗ ██╔╝██╔══██╗██╔══██║
 * ██║  ██║ ╚████╔╝ ██║  ██║██║  ██║
 * ╚═╝  ╚═╝  ╚═══╝  ╚═╝  ╚═╝╚═╝  ╚═╝
 * 
 * Avra: High-Performance, Modern C++20 Audio Plugin & Application Framework
 * Free Non-Commercial Open-Source & Plugin Output License • Real-Time Safe
 */

#if __has_include("avra/core/Types.h")
#include "avra/core/Types.h"
#include "avra/core/AudioSpan.h"
#include "avra/core/Smoother.h"
#include "avra/core/Parameter.h"
#include "avra/core/ParameterTree.h"
#include "avra/core/MidiEvent.h"
#include "avra/core/ProcessContext.h"
#include "avra/core/Processor.h"
#include "avra/core/RingBuffer.h"
#include "avra/core/AudioDeviceOutput.h"
#include "avra/dsp/Math.h"
#include "avra/dsp/SimdBlock.h"
#include "avra/dsp/ModulationMatrix.h"
#include "avra/dsp/BiquadFilter.h"
#include "avra/dsp/DelayLine.h"
#include "avra/dsp/Saturation.h"
#include "avra/dsp/Dynamics.h"
#include "avra/dsp/Lfo.h"
#include "avra/dsp/StereoField.h"
#include "avra/dsp/Oversampling.h"
#include "avra/dsp/Phaser.h"
#include "avra/dsp/circuits/CircuitLibrary.h"

// Hardware-Accelerated GUI Engine
#include "avra/gui/Geometry.h"
#include "avra/gui/Graphics.h"
#include "avra/core/LicenseProvenance.h"
#include "avra/gui/Component.h"
#include "avra/gui/Window.h"
#include "avra/gui/Editor.h"
#include "avra/gui/controls/Knob.h"
#include "avra/gui/controls/Button.h"
#include "avra/gui/controls/Meter.h"
#include "avra/gui/controls/Visualizers.h"
#include "avra/gui/controls/HardwareControls.h"
#else
#include "core/Types.h"
#include "core/AudioSpan.h"
#include "core/Smoother.h"
#include "core/Parameter.h"
#include "core/ParameterTree.h"
#include "core/MidiEvent.h"
#include "core/ProcessContext.h"
#include "core/Processor.h"
#include "core/RingBuffer.h"
#include "core/AudioDeviceOutput.h"
#include "dsp/Math.h"
#include "dsp/SimdBlock.h"
#include "dsp/ModulationMatrix.h"
#include "dsp/BiquadFilter.h"
#include "dsp/DelayLine.h"
#include "dsp/Saturation.h"
#include "dsp/Dynamics.h"
#include "dsp/Lfo.h"
#include "dsp/StereoField.h"
#include "dsp/Oversampling.h"
#include "dsp/Phaser.h"
#include "dsp/circuits/CircuitLibrary.h"

// Hardware-Accelerated GUI Engine
#include "gui/Geometry.h"
#include "gui/Graphics.h"
#include "core/LicenseProvenance.h"
#include "gui/Component.h"
#include "gui/Window.h"
#include "gui/Editor.h"
#include "gui/controls/Knob.h"
#include "gui/controls/Button.h"
#include "gui/controls/Meter.h"
#include "gui/controls/Visualizers.h"
#include "gui/controls/HardwareControls.h"
#endif

