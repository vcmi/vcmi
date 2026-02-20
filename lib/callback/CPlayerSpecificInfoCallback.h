/*
 * CPlayerSpecificInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CGameInfoCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

struct QuestInfo;

class ResourceSet;

class DLL_LINKAGE CPlayerSpecificInfoCallback : public CGameInfoCallback
{
public:
	// keep player-specific override in scope
	using CGameInfoCallback::howManyTowns;

	virtual int howManyTowns() const;
	virtual int howManyHeroes(bool includeGarrisoned = true) const;
	virtual int3 getGrailPos(double *outKnownRatio);

	virtual std::vector <const CGTownInstance *> getTownsInfo(bool onlyOur = true) const; //true -> only owned; false -> all visible
	virtual int getHeroSerial(const CGHeroInstance * hero, bool includeGarrisoned=true) const;
	virtual const CGTownInstance* getTownBySerial(int serialId) const; // serial id is [0, number of towns)
	virtual const CGHeroInstance* getHeroBySerial(int serialId, bool includeGarrisoned=true) const; // serial id is [0, number of heroes)
	virtual std::vector <const CGHeroInstance *> getHeroesInfo() const;
	virtual std::vector <const CGObjectInstance * > getMyObjects() const; //returns all objects flagged by belonging player
	virtual std::vector <QuestInfo> getMyQuests() const;

	virtual int getResourceAmount(GameResID type) const;
	virtual ResourceSet getResourceAmount() const;
};

VCMI_LIB_NAMESPACE_END
