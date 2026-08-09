/*
 * MapViewCache.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Profiler.h"
#include "MapViewCache.h"

#include "IMapRendererContext.h"
#include "MapRenderer.h"
#include "MapViewModel.h"

#include "render/CAnimation.h"
#include "render/Canvas.h"
#include "render/IImage.h"
#include "render/IFont.h"
#include "render/IRenderHandler.h"
#include "render/Graphics.h"

#include "../GameEngine.h"
#include "render/IScreenHandler.h"
#include "../widgets/TextControls.h"

#include "../../lib/int3.h"

MapViewCache::~MapViewCache() = default;

MapViewCache::MapViewCache(const std::shared_ptr<MapViewModel> & model, bool useGpuLayer)
	: model(model)
	, useGpuLayer(useGpuLayer)
	, cachedLevel(0)
	, overlayWasVisible(false)
	, mapRenderer(new MapRenderer())
	, iconsStorage(ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("VwSymbol"), EImageBlitMode::COLORKEY))
{
	Point visibleSize = model->getTilesVisibleDimensions();
	terrainChecksum.resize(boost::extents[visibleSize.x][visibleSize.y]);
	tilesUpToDate.resize(boost::extents[visibleSize.x][visibleSize.y]);
}

std::unique_ptr<Canvas> MapViewCache::createCanvas(const Point & size) const
{
	// a view drawn into the software screen cannot read from a GPU-backed cache
	if(canvasesOnGpu)
		return std::make_unique<Canvas>(ENGINE->screenHandler().createOffscreenCanvas(size));

	return std::make_unique<Canvas>(size, CanvasScalingPolicy::AUTO);
}

void MapViewCache::ensureCanvases()
{
	VCMI_PROFILE_N("MapCache: ensure canvases");
	const bool useGpu = useGpuLayer && ENGINE->screenHandler().isGpuRenderingEnabled();

	// on the GPU the cache keeps its tiles unscaled and scales them while blitting, which spares
	// the per-tile render target the scaling used to go through
	model->setCacheAtNativeSize(useGpu);

	const Point cacheDimensions = model->getCacheDimensionsPixels();

	// only the native cache changes size during play - it follows the number of visible tiles,
	// while the ready-scaled one is measured from the window and stays as it was
	const bool dimensionsStale = useGpu && cachedCanvasDimensions != cacheDimensions;

	if(terrain && canvasesOnGpu == useGpu && !dimensionsStale)
		return;

	// Must run on the rendering thread: this object is constructed while handling a
	// netpack, and creating a texture there would move the GL context off the GUI thread
	canvasesOnGpu = useGpu;
	cachedCanvasDimensions = cacheDimensions;

	intermediate = createCanvas(MapViewModel::getNativeTileSize());
	terrain = createCanvas(cacheDimensions);
	terrainTransition = createCanvas(model->getPixelsVisibleDimensions());

	// the new canvases are empty, so nothing cached about the old ones still holds
	std::fill_n(terrainChecksum.data(), terrainChecksum.num_elements(), TileChecksum{});
	std::fill_n(tilesUpToDate.data(), tilesUpToDate.num_elements(), false);
}

Canvas MapViewCache::getTile(const int3 & coordinates)
{
	return Canvas(*terrain, model->getCacheTileArea(coordinates));
}

std::shared_ptr<IImage> MapViewCache::getOverlayImageForTile(const std::shared_ptr<IMapRendererContext> & context, const int3 & coordinates)
{
	size_t imageIndex = context->overlayImageIndex(coordinates);

	if(imageIndex < iconsStorage->size())
		return iconsStorage->getImage(imageIndex);
	return nullptr;
}

void MapViewCache::invalidate(const std::shared_ptr<IMapRendererContext> & context, const ObjectInstanceID & object)
{
	for(size_t cacheY = 0; cacheY < terrainChecksum.shape()[1]; ++cacheY)
	{
		for(size_t cacheX = 0; cacheX < terrainChecksum.shape()[0]; ++cacheX)
		{
			auto & entry = terrainChecksum[cacheX][cacheY];

			int3 tile(entry.tileX, entry.tileY, cachedLevel);

			if(context->isInMap(tile) && vstd::contains(context->getObjects(tile), object))
				entry = TileChecksum{};
		}
	}
}

/// Terrain animation is unstaggered, so every animated tile goes dirty on the same frame. Drawing
/// them all at once overruns the frame, so the burst is spread over this many frames.
static constexpr int animationCatchUpFrames = 4;

/// Smallest share a frame redraws. The share follows the number of dirty tiles, so it only grows
/// past this when a step dirties more than the old fixed cap could serve before the next one.
static constexpr int animationRedrawShareMinimum = 64;

/// Indices of the terrain and river components of MapRenderer::getTileChecksum()
static constexpr size_t terrainChecksumIndex = 1;
static constexpr size_t riverChecksumIndex = 2;

/// Whether the two checksums differ in nothing but the terrain animation
static bool differsOnlyInTerrainAnimation(const std::array<uint8_t, 8> & before, const std::array<uint8_t, 8> & after)
{
	for(size_t i = 0; i < before.size(); ++i)
		if(before[i] != after[i] && i != terrainChecksumIndex && i != riverChecksumIndex)
			return false;

	return true;
}

void MapViewCache::updateTile(const std::shared_ptr<IMapRendererContext> & context, const int3 & coordinates)
{
	VCMI_PROFILE_N("MapCache: update tile");
	int cacheX = (terrainChecksum.shape()[0] + coordinates.x) % terrainChecksum.shape()[0];
	int cacheY = (terrainChecksum.shape()[1] + coordinates.y) % terrainChecksum.shape()[1];

	auto & oldCacheEntry = terrainChecksum[cacheX][cacheY];
	TileChecksum newCacheEntry;

	newCacheEntry.tileX = coordinates.x;
	newCacheEntry.tileY = coordinates.y;
	newCacheEntry.checksum = mapRenderer->getTileChecksum(*context, coordinates);

	if(cachedLevel == coordinates.z && oldCacheEntry == newCacheEntry && !context->tileAnimated(coordinates))
		return;

	// only an animation step may wait - a scrolled-in or genuinely changed tile is drawn now
	const bool holdsSameTile = oldCacheEntry.tileX == coordinates.x && oldCacheEntry.tileY == coordinates.y;
	const bool animationOnly = cachedLevel == coordinates.z && holdsSameTile
		&& !context->tileAnimated(coordinates)
		&& differsOnlyInTerrainAnimation(oldCacheEntry.checksum, newCacheEntry.checksum);

	if(animationOnly)
	{
		++animatedTilesDirty;

		if(animatedTilesRedrawn >= animatedTileRedrawBudget)
			return;

		++animatedTilesRedrawn;
	}

	Canvas target = getTile(coordinates);

	const uint32_t placeholdersBefore = ENGINE->renderHandler().getPlaceholderDrawCount();

	if(model->getCacheTileSize() == MapViewModel::getNativeTileSize())
	{
		mapRenderer->renderTile(*context, target, coordinates);
	}
	else
	{
		mapRenderer->renderTile(*context, *intermediate, coordinates);
		target.drawScaled(*intermediate, Point(0, 0), model->getCacheTileSize());
	}

	if(context->filterGrayscale())
		target.applyGrayscale();

	// A tile drawn from a stretched stand-in is not final. Leaving its checksum empty keeps
	// it out of the cache, so it is drawn again until the upscale it waits for has finished.
	const bool usedPlaceholder = ENGINE->renderHandler().getPlaceholderDrawCount() != placeholdersBefore;

	oldCacheEntry = usedPlaceholder ? TileChecksum{} : newCacheEntry;
	tilesUpToDate[cacheX][cacheY] = false;
}

void MapViewCache::update(const std::shared_ptr<IMapRendererContext> & context)
{
	VCMI_PROFILE_N("MapCache: update");
	ensureCanvases();

	Rect dimensions = model->getTilesTotalRect();
	bool mapResized = cachedSize != model->getSingleTileSize();

	if(mapResized || dimensions.w != terrainChecksum.shape()[0] || dimensions.h != terrainChecksum.shape()[1])
	{
		boost::multi_array<TileChecksum, 2> newCache;
		newCache.resize(boost::extents[dimensions.w][dimensions.h]);
		terrainChecksum.resize(boost::extents[dimensions.w][dimensions.h]);
		terrainChecksum = newCache;
	}

	if(mapResized || dimensions.w != tilesUpToDate.shape()[0] || dimensions.h != tilesUpToDate.shape()[1])
	{
		boost::multi_array<bool, 2> newCache;
		newCache.resize(boost::extents[dimensions.w][dimensions.h]);
		tilesUpToDate.resize(boost::extents[dimensions.w][dimensions.h]);
		tilesUpToDate = newCache;
	}

	// Refresh whatever the renderer can resolve once instead of per tile
	mapRenderer->prepareFrame(*context);

	const int share = std::max(animationRedrawShareMinimum, (animatedTilesDirtyBefore + animationCatchUpFrames - 1) / animationCatchUpFrames);

	// within a burst the share may only grow - the dirty count falls as tiles are drawn
	animatedTileRedrawBudget = drainingAnimationBurst ? std::max(animatedTileRedrawBudget, share) : share;
	animatedTilesRedrawn = 0;
	animatedTilesDirty = 0;

	for(int y = dimensions.top(); y < dimensions.bottom(); ++y)
		for(int x = dimensions.left(); x < dimensions.right(); ++x)
			updateTile(context, {x, y, model->getLevel()});

	drainingAnimationBurst = animatedTilesDirty > animatedTilesRedrawn;
	animatedTilesDirtyBefore = animatedTilesDirty;

	cachedSize = model->getSingleTileSize();
	cachedLevel = model->getLevel();
	updatedTilesRect = dimensions;
	updatedThisFrame = true;
}

bool MapViewCache::isUpdatedThisFrame() const
{
	return updatedThisFrame
		&& updatedTilesRect == model->getTilesTotalRect()
		&& cachedSize == model->getSingleTileSize()
		&& cachedLevel == model->getLevel();
}

void MapViewCache::renderCachedTiles(Canvas & target)
{
	VCMI_PROFILE_N("MapCache: blit cached tiles");
	const Rect tilesRect = model->getTilesTotalRect();
	const Point tileSize = model->getSingleTileSize();
	const Point cacheTileSize = model->getCacheTileSize();
	const int width = tilesRect.w;
	const int height = tilesRect.h;

	if(width <= 0 || height <= 0)
		return;

	// Screen position of the tile in the top left corner of the visible window, always within
	// one tile of the origin since the view may be scrolled by a fraction of a tile.
	const Point origin = model->getTargetTileArea(int3(tilesRect.x, tilesRect.y, model->getLevel())).topLeft();

	// Position of that same tile inside the cache, which stores tiles wrapped around
	// on both axes (tile x lives at slot x modulo width).
	const int firstSlotX = ((tilesRect.x % width) + width) % width;
	const int firstSlotY = ((tilesRect.y % height) + height) % height;

	// Because of the wrapping the visible window is split into at most two bands per axis - so
	// at most four rectangles, each contiguous both in the cache and on screen.
	const std::array<std::pair<int, int>, 2> columns = {{ // {first slot, slot count}
		{firstSlotX, width - firstSlotX},
		{0, firstSlotX}
	}};
	const std::array<std::pair<int, int>, 2> rows = {{
		{firstSlotY, height - firstSlotY},
		{0, firstSlotY}
	}};

	int offsetX = 0;
	for(const auto & column : columns)
	{
		int offsetY = 0;
		for(const auto & row : rows)
		{
			if(column.second > 0 && row.second > 0)
			{
				Rect cacheArea(
					column.first * cacheTileSize.x,
					row.first * cacheTileSize.y,
					column.second * cacheTileSize.x,
					row.second * cacheTileSize.y);

				Point targetPosition = origin + Point(offsetX * tileSize.x, offsetY * tileSize.y);
				Point targetSize(column.second * tileSize.x, row.second * tileSize.y);

				if(cacheTileSize == tileSize)
					target.draw(Canvas(*terrain, cacheArea), targetPosition);
				else
					target.drawScaled(Canvas(*terrain, cacheArea), targetPosition, targetSize);
			}
			offsetY += row.second;
		}
		offsetX += column.second;
	}
}

void MapViewCache::render(const std::shared_ptr<IMapRendererContext> & context, Canvas & target, bool fullRedraw)
{
	VCMI_PROFILE_N("MapCache: render");
	ensureCanvases();

	bool mapMoved = (cachedPosition != model->getMapViewCenter());
	bool textOverlayVisible = context->showTextOverlay();
	bool overlayVisible = context->showImageOverlay() || textOverlayVisible;
	bool overlayVisibilityChanged = overlayVisible != overlayWasVisible;
	// redraw text overlay backgrounds; track dirty overlay tiles if this becomes expensive.
	bool lazyUpdate = !textOverlayVisible && !overlayVisibilityChanged && !mapMoved && !fullRedraw && vstd::isAlmostZero(context->viewTransitionProgress());

	Rect dimensions = model->getTilesTotalRect();

	if(lazyUpdate)
	{
		// Only the handful of tiles that actually changed need repainting
		for(int y = dimensions.top(); y < dimensions.bottom(); ++y)
		{
			for(int x = dimensions.left(); x < dimensions.right(); ++x)
			{
				int cacheX = (terrainChecksum.shape()[0] + x) % terrainChecksum.shape()[0];
				int cacheY = (terrainChecksum.shape()[1] + y) % terrainChecksum.shape()[1];
				int3 tile(x, y, model->getLevel());

				if(tilesUpToDate[cacheX][cacheY])
					continue;

				Canvas source = getTile(tile);
				Rect targetRect = model->getTargetTileArea(tile);

				if(model->getCacheTileSize() == model->getSingleTileSize())
					target.draw(source, targetRect.topLeft());
				else
					target.drawScaled(source, targetRect.topLeft(), targetRect.dimensions());

				if (!fullRedraw)
					tilesUpToDate[cacheX][cacheY] = true;
			}
		}
	}
	else
	{
		// Every tile has to be repainted - copy the whole cached window at once
		// instead of issuing one blit per tile.
		renderCachedTiles(target);

		if (!fullRedraw)
			std::fill_n(tilesUpToDate.data(), tilesUpToDate.num_elements(), true);
	}

	if(context->showImageOverlay())
	{
		for(int y = dimensions.top(); y < dimensions.bottom(); ++y)
		{
			for(int x = dimensions.left(); x < dimensions.right(); ++x)
			{
				int3 tile(x, y, model->getLevel());
				auto overlay = getOverlayImageForTile(context, tile);

				if(overlay)
				{
					Rect targetRect = model->getTargetTileArea(tile);
					Point position = targetRect.center() - overlay->dimensions() / 2;
					target.draw(overlay, position);
				}
			}
		}
	}

	if(textOverlayVisible)
	{
		const auto & font = ENGINE->renderHandler().loadFont(FONT_TINY);

		for(int y = dimensions.top(); y < dimensions.bottom(); ++y)
		{
			for(int x = dimensions.left(); x < dimensions.right(); ++x)
			{
				int3 tile(x, y, model->getLevel());
				auto overlay = context->overlayText(tile);

				if(!overlay.text.empty())
				{
					Rect targetRect = model->getTargetTileArea(tile);
					Point position = targetRect.center();
					if (x % 2 == 0)
						position.y += targetRect.h / 4;
					else
						position.y -= targetRect.h / 4;

					Point dimensions(font->getStringWidth(overlay.text), font->getLineHeight());
					Rect textRect = Rect(position - dimensions / 2, dimensions).resize(2);

					target.drawColor(textRect, overlay.color);
					target.drawBorder(textRect, Colors::BRIGHT_YELLOW);
					target.drawText(position, EFonts::FONT_TINY, Colors::BLACK, ETextAlignment::CENTER, overlay.text);
				}
			}
		}
	}

	if(!vstd::isAlmostZero(context->viewTransitionProgress()))
		target.drawTransparent(*terrainTransition, Point(0, 0), 1.0 - context->viewTransitionProgress());

	cachedPosition = model->getMapViewCenter();
	overlayWasVisible = overlayVisible;
	updatedThisFrame = false;
}

void MapViewCache::createTransitionSnapshot(const std::shared_ptr<IMapRendererContext> & context)
{
	VCMI_PROFILE_N("MapCache: transition snapshot");
	update(context);
	render(context, *terrainTransition, true);
}
