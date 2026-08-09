/*
 * FrameTimestamps.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

struct SDL_Window;

/// Reports what the display pipeline did with each frame, using EGL_ANDROID_get_frame_timestamps.
///
/// The CPU and GPU zones both end at the swap, so neither can tell a frame that was handed over
/// late from one the compositor simply sat on. Android timestamps every buffer on its way through
/// the queue, which splits that apart:
///
///   requested -> rendering complete   how long our own drawing took to finish
///   rendering complete -> latch       how long a finished buffer waited to be picked up
///   latch -> display present          how long the compositor took to put it on screen
///
/// A stalled swap with a small first number and a large second one is the compositor withholding
/// buffers; the other way round it is us being late.
///
/// Everything here does nothing unless the build has Tracy enabled, the window is EGL backed and
/// the driver implements the extension.
namespace FrameTimestamps
{
/// Enables per-frame timestamps on the window surface. Call on the rendering thread.
void initialize(SDL_Window * window);
void shutdown();

/// Claims the id of the frame that the next swap will produce. Call right before presenting.
void beginFrame();

/// Reports whatever the compositor has finished measuring by now. Call right after presenting.
/// The numbers arrive a few frames late, so they land on the timeline behind the frame they
/// describe - the shape over time is what matters, not the exact alignment.
void collect();

bool isActive();
}
