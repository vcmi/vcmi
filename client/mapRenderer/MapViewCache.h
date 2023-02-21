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
class MapRendererContext;
//class MapViewController;
class MapViewModel;

/// Class responsible for rendering of entire map view
/// uses rendering parameters provided by owner class
class MapViewCache
{
	std::shared_ptr<MapViewModel> model;

	std::unique_ptr<Canvas> terrain;
	std::unique_ptr<Canvas> terrainTransition;
	std::unique_ptr<Canvas> intermediate;
	std::unique_ptr<MapRenderer> mapRenderer;

	std::unique_ptr<CAnimation> iconsStorage;

	Canvas getTile(const int3 & coordinates);
	void updateTile(const std::shared_ptr<MapRendererContext> & context, const int3 & coordinates);

	std::shared_ptr<IImage> getOverlayImageForTile(const std::shared_ptr<MapRendererContext> & context, const int3 & coordinates);
public:
	explicit MapViewCache(const std::shared_ptr<MapViewModel> & model);
	~MapViewCache();

	/// updates internal terrain cache according to provided time delta
	void update(const std::shared_ptr<MapRendererContext> & context);

	/// renders updated terrain cache onto provided canvas
	void render(const std::shared_ptr<MapRendererContext> &context, Canvas & target);
};
