/*
 * Profiler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

/// Tracy instrumentation for the SDL3 media backend. Everything collapses to nothing unless
/// the build is configured with -DENABLE_TRACY=ON, so the macros can be left in the code.

#ifdef VCMI_TRACY

#include <tracy/Tracy.hpp>

/// Zone covering the enclosing scope, named after the function it sits in
#define VCMI_PROFILE() ZoneScoped
/// Zone covering the enclosing scope, under an explicit name
#define VCMI_PROFILE_N(name) ZoneScopedN(name)
/// Attaches a number to the current zone, visible when the zone is selected
#define VCMI_PROFILE_VALUE(value) ZoneValue(value)
/// Attaches a string to the current zone
#define VCMI_PROFILE_TEXT(text) ZoneText((text).c_str(), (text).size())
/// Samples a named metric, drawn as a graph over the timeline
#define VCMI_PROFILE_PLOT(name, value) TracyPlot(name, static_cast<int64_t>(value))
/// Samples a fractional metric - frame rate, milliseconds, ratios
#define VCMI_PROFILE_PLOT_FLOAT(name, value) TracyPlot(name, static_cast<double>(value))
/// Declares how a plot is drawn - call once, before the first sample
#define VCMI_PROFILE_PLOT_MEMORY(name) TracyPlotConfig(name, tracy::PlotFormatType::Memory, true, true, 0)
/// Plot drawn as a smooth line rather than stepped, for continuously changing values
#define VCMI_PROFILE_PLOT_LINE(name) TracyPlotConfig(name, tracy::PlotFormatType::Number, false, true, 0)
/// Ends the current frame - Tracy slices the timeline on these
#define VCMI_PROFILE_FRAME() FrameMark
/// Names the calling thread in the trace
#define VCMI_PROFILE_THREAD(name) tracy::SetThreadName(name)
/// One-off marker on the timeline
#define VCMI_PROFILE_MESSAGE(text) TracyMessageL(text)

#else

#define VCMI_PROFILE()
#define VCMI_PROFILE_N(name)
#define VCMI_PROFILE_VALUE(value)
#define VCMI_PROFILE_TEXT(text)
#define VCMI_PROFILE_PLOT(name, value)
#define VCMI_PROFILE_PLOT_FLOAT(name, value)
#define VCMI_PROFILE_PLOT_MEMORY(name)
#define VCMI_PROFILE_PLOT_LINE(name)
#define VCMI_PROFILE_FRAME()
#define VCMI_PROFILE_THREAD(name)
#define VCMI_PROFILE_MESSAGE(text)

#endif
