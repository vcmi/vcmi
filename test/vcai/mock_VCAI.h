/*
* mock_VCAI.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "gtest/gtest.h"

#include "../AI/VCAI/VCAI.h"

//dependency hell
#include "../../lib/NetPacks.h" //for Component, TryMoveHero
#include "../../lib/serializer/BinarySerializer.h"
#include "../../lib/serializer/BinaryDeserializer.h"

class VCAIMock : public VCAI
{
public:
	VCAIMock();
	virtual ~VCAIMock();

	//overloading all "override" methods from VCAI. AI should never call them, anyway
	MOCK_CONST_METHOD0(getBattleAIName, std::string());

	MOCK_METHOD1(init, void(std::shared_ptr<CCallback> CB));
	MOCK_METHOD0(yourTurn, void());

	MOCK_METHOD4(heroGotLevel, void(const CGHeroInstance * hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill> & skills, QueryID queryID));
	MOCK_METHOD3(commanderGotLevel, void(const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID));
	MOCK_METHOD6(showBlockingDialog, void(const std::string & text, const std::vector<Component> & components, QueryID askID, const int soundID, bool selection, bool cancel, bool safeToAutoaccept));
	MOCK_METHOD4(showGarrisonDialog, void(const CArmedInstance * up, const CGHeroInstance * down, bool removableUnits, QueryID queryID));
	MOCK_METHOD4(showTeleportDialog, void(TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID));
	MOCK_METHOD5(showMapObjectSelectDialog, void(QueryID askID, const Component & icon, const MetaString & title, const MetaString & description, const std::vector<ObjectInstanceID> & objects));
	MOCK_METHOD2(saveGame, void(BinarySerializer & h, const int version));
	MOCK_METHOD2(loadGame, void(BinaryDeserializer & h, const int version));
	MOCK_METHOD0(finish, void());

	MOCK_METHOD1(availableCreaturesChanged, void(const CGDwelling * town));
	MOCK_METHOD1(heroMoved, void(const TryMoveHero & details));
	MOCK_METHOD1(heroInGarrisonChange, void(const CGTownInstance * town));
	MOCK_METHOD2(centerView, void(int3 pos, int focusTime));
	MOCK_METHOD1(tileHidden, void(const std::unordered_set<int3, ShashInt3> & pos));
	MOCK_METHOD2(artifactMoved, void(const ArtifactLocation & src, const ArtifactLocation & dst));
	MOCK_METHOD1(artifactAssembled, void(const ArtifactLocation & al));
	MOCK_METHOD1(showTavernWindow, void(const CGObjectInstance * townOrTavern));
	MOCK_METHOD1(showThievesGuildWindow, void(const CGObjectInstance * obj));
	MOCK_METHOD2(playerBlocked, void(int reason, bool start));
	MOCK_METHOD0(showPuzzleMap, void());
	MOCK_METHOD1(showShipyardDialog, void(const IShipyard * obj));
	MOCK_METHOD2(gameOver, void(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult));
	MOCK_METHOD1(artifactPut, void(const ArtifactLocation & al));
	MOCK_METHOD1(artifactRemoved, void(const ArtifactLocation & al));
	MOCK_METHOD1(artifactDisassembled, void(const ArtifactLocation & al));
	MOCK_METHOD3(heroVisit, void(const CGHeroInstance * visitor, const CGObjectInstance * visitedObj, bool start));
	MOCK_METHOD1(availableArtifactsChanged, void(const CGBlackMarket * bm));
	MOCK_METHOD2(heroVisitsTown, void(const CGHeroInstance * hero, const CGTownInstance * town));
	MOCK_METHOD1(tileRevealed, void(const std::unordered_set<int3, ShashInt3> & pos));
	MOCK_METHOD3(heroExchangeStarted, void(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID query));
	MOCK_METHOD3(heroPrimarySkillChanged, void(const CGHeroInstance * hero, int which, si64 val));
	MOCK_METHOD3(showRecruitmentDialog, void(const CGDwelling * dwelling, const CArmedInstance * dst, int level));
	MOCK_METHOD1(heroMovePointsChanged, void(const CGHeroInstance * hero));
	MOCK_METHOD2(garrisonsChanged, void(ObjectInstanceID id1, ObjectInstanceID id2));
	MOCK_METHOD1(newObject, void(const CGObjectInstance * obj));
	MOCK_METHOD2(showHillFortWindow, void(const CGObjectInstance * object, const CGHeroInstance * visitor));
	MOCK_METHOD2(playerBonusChanged, void(const Bonus & bonus, bool gain));
	MOCK_METHOD1(heroCreated, void(const CGHeroInstance *));
	MOCK_METHOD2(advmapSpellCast, void(const CGHeroInstance * caster, int spellID));
	MOCK_METHOD3(showInfoDialog, void(const std::string & text, const std::vector<Component> & components, int soundID));
	MOCK_METHOD1(requestRealized, void(PackageApplied * pa));
	MOCK_METHOD0(receivedResource, void());
	MOCK_METHOD1(objectRemoved, void(const CGObjectInstance * obj));
	MOCK_METHOD2(showUniversityWindow, void(const IMarket * market, const CGHeroInstance * visitor));
	MOCK_METHOD1(heroManaPointsChanged, void(const CGHeroInstance * hero));
	MOCK_METHOD3(heroSecondarySkillChanged, void(const CGHeroInstance * hero, int which, int val));
	MOCK_METHOD0(battleResultsApplied, void());
	MOCK_METHOD0(battleEnded, void());
	MOCK_METHOD1(objectPropertyChanged, void(const SetObjectProperty * sop));
	MOCK_METHOD3(buildChanged, void(const CGTownInstance * town, BuildingID buildingID, int what));
	MOCK_METHOD3(heroBonusChanged, void(const CGHeroInstance * hero, const Bonus & bonus, bool gain));
	MOCK_METHOD2(showMarketWindow, void(const IMarket * market, const CGHeroInstance * visitor));
	MOCK_METHOD1(showWorldViewEx, void(const std::vector<ObjectPosInfo> & objectPositions));

	MOCK_METHOD6(battleStart, void(const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2, bool side));
	MOCK_METHOD1(battleEnd, void(const BattleResult * br));

	MOCK_METHOD2(requestSent, void(const CPackForServer * pack, int requestID));

	//interetsing stuff

	MOCK_CONST_METHOD0(getFlaggedObjects, std::vector<const CGObjectInstance *>());

};
