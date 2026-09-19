/*
 * MapViewCache.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/Point.h"
#include "../../lib/Rect.h"

class ObjectInstanceID;

class IImage;
class CAnimation;
class Canvas;
class MapRenderer;
class IMapRendererContext;
class MapViewModel;

/// Class responsible for rendering of entire map view
/// uses rendering parameters provided by owner class
class MapViewCache
{
	struct TileChecksum
	{
		int tileX = std::numeric_limits<int>::min();
		int tileY = std::numeric_limits<int>::min();
		std::array<uint8_t, 8> checksum{};

		bool operator==(const TileChecksum & other) const
		{
			return tileX == other.tileX && tileY == other.tileY && checksum == other.checksum;
		}
	};

	boost::multi_array<TileChecksum, 2> terrainChecksum;
	boost::multi_array<bool, 2> tilesUpToDate;

	/// Animation-step bookkeeping of update(): tiles dirtied this frame and the previous one,
	/// tiles redrawn so far, this frame's share, and whether the previous frame left some waiting
	int animatedTilesDirty = 0;
	int animatedTilesDirtyBefore = 0;
	int animatedTilesRedrawn = 0;
	int animatedTileRedrawBudget = 0;
	bool drainingAnimationBurst = false;

	Point cachedSize;
	Point cachedPosition;
	int cachedLevel;
	bool overlayWasVisible;

	std::shared_ptr<MapViewModel> model;

	/// Whether this cache may allocate GPU render targets
	bool useGpuLayer;

	/// Whether the canvases below are GPU backed. A colour scheme moves the whole client to the
	/// surface path, and a cache built for one path cannot be drawn onto the other.
	bool canvasesOnGpu = false;

	/// Whether the terrain cache was last built at native tile size, rather than the bounded,
	/// ready-scaled size used when that would exceed the driver's maximum texture size
	bool cachedAtNativeSize = false;

	/// Size the cache canvas was created for - at native tile size it follows the zoom level
	Point cachedCanvasDimensions;

	/// Set by update(), cleared by render(). Lets a caller that already filled the cache
	/// this frame skip doing it again, which would re-checksum every visible tile.
	bool updatedThisFrame = false;

	/// Tiles the cache was filled for, to detect a scroll between the update and the draw
	Rect updatedTilesRect;

	std::unique_ptr<Canvas> terrain;
	std::unique_ptr<Canvas> terrainTransition;
	std::unique_ptr<Canvas> intermediate;
	std::unique_ptr<MapRenderer> mapRenderer;

	/// Canvas of the given logical size, drawing onto a GPU render target when this view
	/// uses the GPU layer and onto a plain surface otherwise
	std::unique_ptr<Canvas> createCanvas(const Point & size) const;

	/// Allocates the canvases on first use. Deferred out of the constructor because that
	/// runs on the network thread, where creating a texture would steal the GL context
	void ensureCanvases();

	std::shared_ptr<CAnimation> iconsStorage;

	Canvas getTile(const int3 & coordinates);
	void updateTile(const std::shared_ptr<IMapRendererContext> & context, const int3 & coordinates);

	/// Walks the visible window of the cache. Because tiles are stored wrapped around on both
	/// axes it falls into at most four bands, each contiguous in the cache and on screen.
	void forEachCachedBand(const std::function<void(const Rect & cacheArea, const Point & targetPosition, const Point & targetSize)> & visit) const;

	/// Copies the entire cached tile window onto the target in as few blits as possible. Used
	/// when every tile has to be repainted anyway, above all while the view is scrolling.
	void renderCachedTiles(Canvas & target);

	std::shared_ptr<IImage> getOverlayImageForTile(const std::shared_ptr<IMapRendererContext> & context, const int3 & coordinates);

public:
	/// useGpuLayer must match how the owning view presents itself: a view drawn into the
	/// software screen cannot read from a GPU-backed cache
	MapViewCache(const std::shared_ptr<MapViewModel> & model, bool useGpuLayer);
	~MapViewCache();

	/// invalidates cache of specified object
	void invalidate(const std::shared_ptr<IMapRendererContext> & context, const ObjectInstanceID & object);

	/// updates internal terrain cache according to provided time delta
	/// True when update() already ran this frame and the view still shows the same tiles. A scroll
	/// in between brings tiles into view whose cache slots were never filled for them.
	bool isUpdatedThisFrame() const;

	void update(const std::shared_ptr<IMapRendererContext> & context);

	/// renders updated terrain cache onto provided canvas
	void render(const std::shared_ptr<IMapRendererContext> & context, Canvas & target, bool fullRedraw);

	/// Hands the visible window of the cache to the screen handler, to be drawn onto the screen
	/// while the frame is composed rather than copied into a layer now. Only valid when nothing
	/// has to be drawn on top of the terrain - see needsOwnLayer(). targetArea is in screen pixels.
	void present(const Rect & targetArea);

	/// Whether this frame draws anything over the terrain, which needs a layer of its own
	bool needsOwnLayer(const std::shared_ptr<IMapRendererContext> & context) const;

	/// creates snapshot of current view and stores it into internal canvas
	/// used for view transition, e.g. Dimension Door spell or teleporters (Subterra gates / Monolith)
	void createTransitionSnapshot(const std::shared_ptr<IMapRendererContext> & context);
};
