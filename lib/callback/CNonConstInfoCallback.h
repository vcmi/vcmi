/*
 * CNonConstInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CGameInfoCallback.h"

#include <vcmi/Metatype.h>

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

class DLL_LINKAGE CNonConstInfoCallback : public CGameInfoCallback
{
public:
	//keep const version of callback accessible
	using CGameInfoCallback::getPlayerState;
	using CGameInfoCallback::getTeam;
	using CGameInfoCallback::getPlayerTeam;
	using CGameInfoCallback::getHero;
	using CGameInfoCallback::getTown;
	using CGameInfoCallback::getTile;
	using CGameInfoCallback::getArtInstance;
	using CGameInfoCallback::getObjInstance;
	using CGameInfoCallback::getArtSet;

	PlayerState * getPlayerState(const PlayerColor & color, bool verbose = true);
	TeamState * getTeam(const TeamID & teamID); //get team by team ID
	TeamState * getPlayerTeam(const PlayerColor & color); // get team by player color
	CGHeroInstance * getHero(const ObjectInstanceID & objid);
	CGTownInstance * getTown(const ObjectInstanceID & objid);
	TerrainTile * getTile(const int3 & pos);
	CArtifactInstance * getArtInstance(const ArtifactInstanceID & aid);
	CGObjectInstance * getObjInstance(const ObjectInstanceID & oid);
	CArmedInstance * getArmyInstance(const ObjectInstanceID & oid);
	CArtifactSet * getArtSet(const ArtifactLocation & loc);

	virtual void updateEntity(Metatype metatype, int32_t index, const JsonNode & data) = 0;
};

VCMI_LIB_NAMESPACE_END
