/*
 * MapInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IGameInfoCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMap;

class DLL_LINKAGE MapInfoCallback : public IGameInfoCallback
{
public:
	virtual ~MapInfoCallback();
	virtual const CMap * getMapConstPtr() const = 0;

	const CGObjectInstance * getObj(ObjectInstanceID objid, bool verbose = true) const override;
	const CGObjectInstance * getObjInstance(ObjectInstanceID oid) const override;
	const CArtifactInstance * getArtInstance(ArtifactInstanceID aid) const override;
	const CGHeroInstance * getHero(ObjectInstanceID objid) const override;
	const CGTownInstance * getTown(ObjectInstanceID objid) const override;
	PlayerColor getOwner(ObjectInstanceID heroID) const;

	bool isInTheMap(const int3 & pos) const override;
	bool isAllowed(SpellID id) const override;
	bool isAllowed(ArtifactID id) const override;
	bool isAllowed(SecondarySkill id) const override;
	int3 getMapSize() const override;
	void getAllowedSpells(std::vector<SpellID> & out, std::optional<ui16> level);

	const IGameSettings & getSettings() const override;
	const CMapHeader * getMapHeader() const override;

};

VCMI_LIB_NAMESPACE_END
