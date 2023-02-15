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

VCMI_LIB_NAMESPACE_BEGIN

class int3;
class ObjectInstanceID;
class CGObjectInstance;

VCMI_LIB_NAMESPACE_END

class CAnimation;
class IImage;
class Canvas;
class IMapRendererContext;

class MapObjectsSorter
{
	const IMapRendererContext & context;
public:
	explicit MapObjectsSorter(const IMapRendererContext & context);

	bool operator ()(const ObjectInstanceID & left, const ObjectInstanceID & right) const;
	bool operator ()(const CGObjectInstance * left, const CGObjectInstance * right) const;
};

class MapTileStorage
{
	using TerrainAnimation = std::array<std::unique_ptr<CAnimation>, 4>;
	std::vector<TerrainAnimation> animations;
public:
	explicit MapTileStorage( size_t capacity);
	void load(size_t index, const std::string& filename);
	std::shared_ptr<IImage> find(size_t fileIndex, size_t rotationIndex, size_t imageIndex );
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
	using MapObject = ObjectInstanceID;
	using MapTile   = std::vector<MapObject>;

	boost::multi_array<MapTile, 3> objects;
	std::map<std::string, std::shared_ptr<CAnimation>> animations;

	std::shared_ptr<CAnimation> getFlagAnimation(const CGObjectInstance* obj);
	std::shared_ptr<CAnimation> getAnimation(const CGObjectInstance* obj);
	std::shared_ptr<CAnimation> getAnimation(const std::string & filename);

	std::shared_ptr<IImage> getImage(const IMapRendererContext & context, const CGObjectInstance * obj, const std::shared_ptr<CAnimation>& animation) const;
	size_t getAnimationGroup(const IMapRendererContext & context, const CGObjectInstance * obj) const;

	void initializeObjects(const IMapRendererContext & context);
	void renderImage(Canvas & target, const int3 & coordinates, const CGObjectInstance * object, const std::shared_ptr<IImage>& image);

	void renderObject(const IMapRendererContext & context, Canvas & target, const int3 & coordinates, const CGObjectInstance* obj);
public:
	explicit MapRendererObjects(const IMapRendererContext & context);
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);

	void addObject(const IMapRendererContext & context, const CGObjectInstance * object);
	void removeObject(const IMapRendererContext & context, const CGObjectInstance * object);
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


class MapRendererDebugGrid
{
public:
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
	MapRendererDebugGrid rendererDebugGrid;

public:
	explicit MapRenderer(const IMapRendererContext & context);

	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);

	void addObject(const IMapRendererContext & context, const CGObjectInstance * object);
	void removeObject(const IMapRendererContext & context, const CGObjectInstance * object);

};
