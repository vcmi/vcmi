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
enum class EImageBlitMode : uint8_t;

class MapTileStorage
{
	using TerrainAnimation = std::array<std::unique_ptr<CAnimation>, 4>;
	std::vector<TerrainAnimation> animations;

public:
	explicit MapTileStorage(size_t capacity);
	void load(size_t index, const std::string & filename, EImageBlitMode blitMode);
	std::shared_ptr<IImage> find(size_t fileIndex, size_t rotationIndex, size_t imageIndex);
};

class MapRendererTerrain
{
	MapTileStorage storage;

public:
	MapRendererTerrain();

	uint8_t checksum(const IMapRendererContext & context, const int3 & coordinates);
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererRiver
{
	MapTileStorage storage;

public:
	MapRendererRiver();

	uint8_t checksum(const IMapRendererContext & context, const int3 & coordinates);
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererRoad
{
	MapTileStorage storage;

public:
	MapRendererRoad();

	uint8_t checksum(const IMapRendererContext & context, const int3 & coordinates);
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererObjects
{
	std::unordered_map<std::string, std::shared_ptr<CAnimation>> animations;

	std::shared_ptr<CAnimation> getBaseAnimation(const CGObjectInstance * obj);
	std::shared_ptr<CAnimation> getFlagAnimation(const CGObjectInstance * obj);
	std::shared_ptr<CAnimation> getOverlayAnimation(const CGObjectInstance * obj);

	std::shared_ptr<CAnimation> getAnimation(const std::string & filename, bool generateMovementGroups);

	std::shared_ptr<IImage> getImage(const IMapRendererContext & context, const CGObjectInstance * obj, const std::shared_ptr<CAnimation> & animation) const;

	void renderImage(const IMapRendererContext & context, Canvas & target, const int3 & coordinates, const CGObjectInstance * object, const std::shared_ptr<IImage> & image);
	void renderObject(const IMapRendererContext & context, Canvas & target, const int3 & coordinates, const CGObjectInstance * obj);

public:
	uint8_t checksum(const IMapRendererContext & context, const int3 & coordinates);
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererBorder
{
	std::unique_ptr<CAnimation> animation;

	size_t getIndexForTile(const IMapRendererContext & context, const int3 & coordinates);

public:
	MapRendererBorder();

	uint8_t checksum(const IMapRendererContext & context, const int3 & coordinates);
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererFow
{
	std::unique_ptr<CAnimation> fogOfWarFullHide;
	std::unique_ptr<CAnimation> fogOfWarPartialHide;

public:
	MapRendererFow();

	uint8_t checksum(const IMapRendererContext & context, const int3 & coordinates);
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererPath
{
	std::unique_ptr<CAnimation> pathNodes;

	size_t selectImageReachability(bool reachableToday, size_t imageIndex);
	size_t selectImageCross(bool reachableToday, const int3 & curr);
	size_t selectImageArrow(bool reachableToday, const int3 & curr, const int3 & prev, const int3 & next);
	size_t selectImage(const IMapRendererContext & context, const int3 & coordinates);

public:
	MapRendererPath();

	uint8_t checksum(const IMapRendererContext & context, const int3 & coordinates);
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererDebug
{
	std::shared_ptr<IImage> imageGrid;
	std::shared_ptr<IImage> imageVisitable;
	std::shared_ptr<IImage> imageBlockable;
public:
	MapRendererDebug();

	uint8_t checksum(const IMapRendererContext & context, const int3 & coordinates);
	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};

class MapRendererOverlay
{
	std::unique_ptr<CAnimation> iconsStorage;
public:
	MapRendererOverlay();

	uint8_t checksum(const IMapRendererContext & context, const int3 & coordinates);
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
	using TileChecksum = std::array<uint8_t, 8>;

	TileChecksum getTileChecksum(const IMapRendererContext & context, const int3 & coordinates);

	void renderTile(const IMapRendererContext & context, Canvas & target, const int3 & coordinates);
};
