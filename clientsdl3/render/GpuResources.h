/*
 * GpuResources.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "lib/Point.h"

#include "TextTextureCache.h"

struct SDL_Renderer;
struct SDL_Texture;

/// Everything that belongs to the one renderer and dies with it: the renderer itself, the caches
/// keyed on it and the textures it still has to free. Owned by ScreenHandler, which is what
/// creates and destroys the renderer.
///
/// get() is a Meyer's singleton, not a GameEngine member, on purpose: this codebase has no
/// enforced invariant that every object touching a texture is destroyed before GameEngine is,
/// and a cached image can end up outliving it through a path that is not obvious at a glance -
/// a stray shared_ptr held by a file-scope cache elsewhere, a callback, anything. A function-
/// local static sidesteps that entirely by living until actual process exit, in the same
/// teardown phase as every other stray static in the program, GameEngine's members included.
class GpuResources
{
	SDL_Renderer * mainRenderer = nullptr;

	/// Raised whenever the renderer is destroyed. Textures remember the value they were created
	/// under, so a stale one tells its owner the pointer is dangling and must be dropped, not freed.
	uint32_t rendererGeneration = 1;

	/// Scratch texture that surface-backed canvases are uploaded through - one per copy would cost
	/// a GPU allocation and a full upload every time, so a single one is grown and reused.
	/// SDL_UpdateTexture flushes any batch still referring to it, so several copies per frame work.
	SDL_Texture * uploadTexture = nullptr;
	Point uploadTextureSize;
	uint32_t uploadTextureGeneration = 0;

	std::mutex pendingTextureMutex;
	std::vector<SDL_Texture *> pendingTextureDestruction;

	TextTextureCache textTextureCache;

public:
	static GpuResources & get();

	SDL_Renderer * renderer() const { return mainRenderer; }
	uint32_t generation() const { return rendererGeneration; }

	TextTextureCache & textCache() { return textTextureCache; }

	void setRenderer(SDL_Renderer * renderer);

	/// Drops the renderer and everything that was created from it
	void destroyRenderer();

	/// Upload texture of at least the requested size, or null if it could not be created
	SDL_Texture * acquireUploadTexture(const Point & size);

	/// Queues a texture for destruction on the rendering thread - owners are released on whichever
	/// thread drops them, and a texture may only be destroyed on the thread that owns the renderer.
	void destroyTextureDeferred(SDL_Texture * texture);

	/// Frees what destroyTextureDeferred() collected and returns how many that was.
	/// Rendering thread only.
	size_t processPendingTextureDestruction();
};
