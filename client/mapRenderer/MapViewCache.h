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

VCMI_LIB_NAMESPACE_BEGIN
class int3;
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
		std::array<uint8_t, 8> checksum {};

		bool operator == (const TileChecksum & other) const
		{
			return tileX == other.tileX && tileY == other.tileY && checksum == other.checksum;
		}
	};

	boost::multi_array<TileChecksum, 2> terrainChecksum;
	int cachedLevel;

	std::shared_ptr<MapViewModel> model;

	std::unique_ptr<Canvas> terrain;
	std::unique_ptr<Canvas> terrainTransition;
	std::unique_ptr<Canvas> intermediate;
	std::unique_ptr<MapRenderer> mapRenderer;

	std::unique_ptr<CAnimation> iconsStorage;

	Canvas getTile(const int3 & coordinates);
	void updateTile(const std::shared_ptr<const IMapRendererContext> & context, const int3 & coordinates);

	std::shared_ptr<IImage> getOverlayImageForTile(const std::shared_ptr<const IMapRendererContext> & context, const int3 & coordinates);
public:
	explicit MapViewCache(const std::shared_ptr<MapViewModel> & model);
	~MapViewCache();

	/// updates internal terrain cache according to provided time delta
	void update(const std::shared_ptr<const IMapRendererContext> & context);

	/// renders updated terrain cache onto provided canvas
	void render(const std::shared_ptr<const IMapRendererContext> &context, Canvas & target);
};
