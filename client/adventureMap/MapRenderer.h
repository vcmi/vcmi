/*
 * MapRenderer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "MapRendererContext.h"

VCMI_LIB_NAMESPACE_BEGIN

class int3;
class ObjectInstanceID;
class CGObjectInstance;

VCMI_LIB_NAMESPACE_END

class CAnimation;
class IImage;
class Canvas;

class MapObjectsSorter
{
	const IMapRendererContext & context;

public:
	explicit MapObjectsSorter(const IMapRendererContext & context);

	bool operator()(const ObjectInstanceID & left, const ObjectInstanceID & right) const;
	bool operator()(const CGObjectInstance * left, const CGObjectInstance * right) const;
};

class MapTileStorage
{
	using TerrainAnimation = std::array<std::unique_ptr<CAnimation>, 4>;
	std::vector<TerrainAnimation> animations;

public:
	explicit MapTileStorage(size_t capacity);
	void load(size_t index, const std::string & filename);
	std::shared_ptr<IImage> find(size_t fileIndex, size_t rotationIndex, size_t imageIndex);
};

class MapRendererTerrain
{
	MapTileStorage storage;

public:
	MapRendererTerrain();
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererRiver
{
	MapTileStorage storage;

public:
	MapRendererRiver();
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererRoad
{
	MapTileStorage storage;

public:
	MapRendererRoad();
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererObjects
{
	std::map<std::string, std::shared_ptr<CAnimation>> animations;

	std::shared_ptr<CAnimation> getBaseAnimation(const CGObjectInstance * obj);
	std::shared_ptr<CAnimation> getFlagAnimation(const CGObjectInstance * obj);
	std::shared_ptr<CAnimation> getOverlayAnimation(const CGObjectInstance * obj);

	std::shared_ptr<CAnimation> getAnimation(const std::string & filename, bool generateMovementGroups);

	std::shared_ptr<IImage> getImage(const IMapRendererContext & context, const CGObjectInstance * obj, const std::shared_ptr<CAnimation> & animation) const;

	void renderImage(const IMapRendererContext & context, Canvas & target, const int3 & coordinates, const CGObjectInstance * object, const std::shared_ptr<IImage> & image);
	void renderObject(const IMapRendererContext & context, Canvas & target, const int3 & coordinates, const CGObjectInstance * obj);

public:
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererBorder
{
	std::unique_ptr<CAnimation> animation;

	size_t getIndexForTile(const IMapRendererContext & context, const int3 & coordinates);

public:
	MapRendererBorder();
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererFow
{
	std::unique_ptr<CAnimation> fogOfWarFullHide;
	std::unique_ptr<CAnimation> fogOfWarPartialHide;

public:
	MapRendererFow();
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererPath
{
	std::unique_ptr<CAnimation> pathNodes;

	void renderImage(Canvas & target, bool reachableToday, size_t imageIndex);
	void renderImageCross(Canvas & target, bool reachableToday, const int3 & curr);
	void renderImageArrow(Canvas & target, bool reachableToday, const int3 & curr, const int3 & prev, const int3 & next);

public:
	MapRendererPath();
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererDebug
{
	std::shared_ptr<IImage> imageGrid;
	std::shared_ptr<IImage> imageVisitable;
	std::shared_ptr<IImage> imageBlockable;
public:
	MapRendererDebug();

	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRenderer
{
	MapRendererTerrain rendererTerrain;
	MapRendererRiver rendererRiver;
	MapRendererRoad rendererRoad;
	MapRendererBorder rendererBorder;
	MapRendererFow rendererFow;
	MapRendererObjects rendererObjects;
	MapRendererPath rendererPath;
	MapRendererDebug rendererDebug;

public:
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};
