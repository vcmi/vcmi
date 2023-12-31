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

VCMI_LIB_NAMESPACE_BEGIN
class ObjectInstanceID;
VCMI_LIB_NAMESPACE_END

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

	Point cachedSize;
	Point cachedPosition;
	int cachedLevel;

	std::shared_ptr<MapViewModel> model;

	std::unique_ptr<Canvas> terrain;
	std::unique_ptr<Canvas> terrainTransition;
	std::unique_ptr<Canvas> intermediate;
	std::unique_ptr<MapRenderer> mapRenderer;

	std::shared_ptr<CAnimation> iconsStorage;

	Canvas getTile(const int3 & coordinates);
	void updateTile(const std::shared_ptr<IMapRendererContext> & context, const int3 & coordinates);

	std::shared_ptr<IImage> getOverlayImageForTile(const std::shared_ptr<IMapRendererContext> & context, const int3 & coordinates);

public:
	explicit MapViewCache(const std::shared_ptr<MapViewModel> & model);
	~MapViewCache();

	/// invalidates cache of specified object
	void invalidate(const std::shared_ptr<IMapRendererContext> & context, const ObjectInstanceID & object);

	/// updates internal terrain cache according to provided time delta
	void update(const std::shared_ptr<IMapRendererContext> & context);

	/// renders updated terrain cache onto provided canvas
	void render(const std::shared_ptr<IMapRendererContext> & context, Canvas & target, bool fullRedraw);

	/// creates snapshot of current view and stores it into internal canvas
	/// used for view transition, e.g. Dimension Door spell or teleporters (Subterra gates / Monolith)
	void createTransitionSnapshot(const std::shared_ptr<IMapRendererContext> & context);
};
