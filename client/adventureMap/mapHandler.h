/*
 * mapHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

#include "../../lib/int3.h"
#include "../../lib/spells/ViewSpellInt.h"
#include "../../lib/Rect.h"


#ifdef IN
#undef IN
#endif

#ifdef OUT
#undef OUT
#endif

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class CGHeroInstance;
class CGBoat;
class CMap;
struct TerrainTile;
class PlayerColor;

VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
class CAnimation;
class IImage;
class CFadeAnimation;
class CMapHandler;
class IMapObjectObserver;

enum class EWorldViewIcon
{
	TOWN = 0,
	HERO,
	ARTIFACT,
	TELEPORT,
	GATE,
	MINE_WOOD,
	MINE_MERCURY,
	MINE_STONE,
	MINE_SULFUR,
	MINE_CRYSTAL,
	MINE_GEM,
	MINE_GOLD,
	RES_WOOD,
	RES_MERCURY,
	RES_STONE,
	RES_SULFUR,
	RES_CRYSTAL,
	RES_GEM,
	RES_GOLD,

};

enum class EMapObjectFadingType
{
	NONE,
	IN,
	OUT
};

struct TerrainTileObject
{
	const CGObjectInstance *obj;
	Rect rect;
	int fadeAnimKey;
	boost::optional<std::string> ambientSound;

	TerrainTileObject(const CGObjectInstance *obj_, Rect rect_, bool visitablePos = false);
	~TerrainTileObject();
};

struct TerrainTile2
{
	std::vector<TerrainTileObject> objects; //pointers to objects being on this tile with rects to be easier to blit this tile on screen
};

class CMapHandler
{
	const CMap * map;
	std::vector<IMapObjectObserver *> observers;

public:
	explicit CMapHandler(const CMap * map);

	const CMap * getMap();

	/// returns true if tile is within map bounds
	bool isInMap(const int3 & tile);

	/// see MapObjectObserver interface
	void onObjectFadeIn(const CGObjectInstance * obj);
	void onObjectFadeOut(const CGObjectInstance * obj);
	void onObjectInstantAdd(const CGObjectInstance * obj);
	void onObjectInstantRemove(const CGObjectInstance * obj);
	void onHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest);
	void onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest);
	void onHeroRotated(const CGHeroInstance * obj, const int3 & from, const int3 & dest);

	/// Add object to receive notifications on any changes in visible map state
	void addMapObserver(IMapObjectObserver * observer);
	void removeMapObserver(IMapObjectObserver * observer);

	/// returns string description for terrain interaction
	void getTerrainDescr(const int3 & pos, std::string & out, bool isRMB) const;

	/// returns list of ambient sounds for specified tile
	std::vector<std::string> getAmbientSounds(const int3 & tile);

	/// returns true if tile has hole from grail digging attempt
	bool hasObjectHole(const int3 & pos) const;

	/// determines if the map is ready to handle new hero movement (not available during fading animations)
	bool hasActiveAnimations();

	static bool compareObjectBlitOrder(const CGObjectInstance * a, const CGObjectInstance * b);
};
