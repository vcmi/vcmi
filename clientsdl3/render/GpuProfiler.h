/*
 * GpuProfiler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

struct SDL_Texture;

/// GPU-side timing for the SDL3 backend, reported to Tracy next to the CPU zones.
///
/// SDL flushes its command queue whenever the render target changes, so one zone per bound
/// target covers exactly one GPU pass. That is the granularity the cost of a tiled mobile
/// GPU shows up at: every pass loads its target into tile memory and resolves it back out,
/// which no CPU-side measurement can see.
///
/// Everything here does nothing unless the build has Tracy enabled, the renderer is an
/// OpenGL or OpenGL ES one, and the driver implements timer queries.
namespace GpuProfiler
{
/// Sets up the Tracy GPU context. Call on the rendering thread once the renderer exists.
void initialize();

/// Stops emitting zones. Tracy cannot tear a GPU context down, so profiling stays off for
/// the rest of the run - call this before the renderer, and with it the GL context, dies.
void shutdown();

/// Gives a render target a name to show on the timeline. Targets left unnamed are reported
/// as offscreen passes. The name is not copied, so it has to outlive the target.
void nameTarget(SDL_Texture * target, const char * name);
void forgetTarget(SDL_Texture * target);

/// Closes the pass that was open and opens one for the target that is now bound. Call right
/// after SDL_SetRenderTarget, which is where SDL submits the previous pass to the driver.
void beginPass(SDL_Texture * target);

/// Closes the open pass without opening another
void endPass();

/// Reads back whatever timestamps the GPU has finished with. Call once per frame.
void collect();

bool isActive();
}
