/*
* mock_CPLayerSpecificInfoCallback.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "gtest/gtest.h"

#include "../lib/CGameInfoCallback.h"
#include "../lib/ResourceSet.h"

class GameCallbackMock : public CPlayerSpecificInfoCallback
{
public:
	//int howManyTowns() const;
	//int howManyHeroes(bool includeGarrisoned = true) const;
	//int3 getGrailPos(double *outKnownRatio);
	//boost::optional<PlayerColor> getMyColor() const;

	//int getHeroSerial(const CGHeroInstance * hero, bool includeGarrisoned = true) const;
	//const CGTownInstance* getTownBySerial(int serialId) const; // serial id is [0, number of towns)
	//const CGHeroInstance* getHeroBySerial(int serialId, bool includeGarrisoned = true) const; // serial id is [0, number of heroes)
	//std::vector <const CGHeroInstance *> getHeroesInfo(bool onlyOur = true) const; //true -> only owned; false -> all visible
	//std::vector <const CGDwelling *> getMyDwellings() const; //returns all dwellings that belong to player
	//std::vector <const CGObjectInstance * > getMyObjects() const; //returns all objects flagged by belonging player
	//std::vector <QuestInfo> getMyQuests() const;

	//int getResourceAmount(Res::ERes type) const;
	//const std::vector< std::vector< std::vector<ui8> > > & getVisibilityMap()const; //returns visibility map
	//const PlayerSettings * getPlayerSettings(PlayerColor color) const;

	//TResources getResourceAmount() const;
	//std::vector <const CGTownInstance *> getTownsInfo(bool onlyOur = true) const; //true -> only owned; false -> all visible

	MOCK_CONST_METHOD0(getResourceAmount, TResources());
	MOCK_CONST_METHOD1(getTownsInfo, std::vector <const CGTownInstance *>(bool));
};