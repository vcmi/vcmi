/*
 * GpuResources.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "GpuResources.h"

#include <SDL3/SDL.h>

GpuResources & GpuResources::get()
{
	static GpuResources instance;
	return instance;
}

void GpuResources::setRenderer(SDL_Renderer * renderer)
{
	mainRenderer = renderer;
}

void GpuResources::destroyRenderer()
{
	if(!mainRenderer)
		return;

	SDL_DestroyRenderer(mainRenderer);
	mainRenderer = nullptr;

	// every texture created from the old renderer is now dangling - the upload texture went with
	// it, and the generation tells every other owner to drop its pointer instead of freeing it
	uploadTexture = nullptr;
	++rendererGeneration;
}

SDL_Texture * GpuResources::acquireUploadTexture(const Point & size)
{
	const bool sameRenderer = uploadTexture && uploadTextureGeneration == rendererGeneration;

	if(sameRenderer && uploadTextureSize.x >= size.x && uploadTextureSize.y >= size.y)
		return uploadTexture;

	// never shrink - a smaller region can always be uploaded into a corner of a larger texture
	const Point wanted = sameRenderer ? Point(std::max(size.x, uploadTextureSize.x), std::max(size.y, uploadTextureSize.y)) : size;

	SDL_Texture * created = SDL_CreateTexture(mainRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, wanted.x, wanted.y);

	if(!created)
		return nullptr;

	// textures of a destroyed renderer are gone with it and must not be touched
	if(sameRenderer)
		SDL_DestroyTexture(uploadTexture);

	uploadTexture = created;
	uploadTextureSize = wanted;
	uploadTextureGeneration = rendererGeneration;

	return uploadTexture;
}

void GpuResources::destroyTextureDeferred(SDL_Texture * texture)
{
	if(!texture)
		return;

	std::lock_guard lock(pendingTextureMutex);
	pendingTextureDestruction.push_back(texture);
}

size_t GpuResources::processPendingTextureDestruction()
{
	std::lock_guard lock(pendingTextureMutex);

	const size_t destroyed = pendingTextureDestruction.size();

	for(SDL_Texture * texture : pendingTextureDestruction)
		SDL_DestroyTexture(texture);

	pendingTextureDestruction.clear();

	return destroyed;
}
