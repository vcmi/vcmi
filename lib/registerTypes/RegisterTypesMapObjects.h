/*
 * RegisterTypesMapObjects.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../mapObjectConstructors/CBankInstanceConstructor.h"
#include "../mapObjects/MapObjects.h"
#include "../mapObjects/CGCreature.h"
#include "../mapObjects/CGTownBuilding.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../battle/BattleInfo.h"
#include "../battle/CObstacleInstance.h"
#include "../bonuses/Limiters.h"
#include "../bonuses/Updaters.h"
#include "../bonuses/Propagators.h"
#include "../CPlayerState.h"
#include "../CStack.h"
#include "../CHeroHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

template<typename Serializer>
void registerTypesMapObjects(Serializer &s)
{
	//////////////////////////////////////////////////////////////////////////
	// Adventure map objects
	//////////////////////////////////////////////////////////////////////////
	s.template registerType<IObjectInterface, CGObjectInstance>();

	// Non-armed objects
	s.template registerType<CGObjectInstance, CGTeleport>();
		s.template registerType<CGTeleport, CGMonolith>();
			s.template registerType<CGMonolith, CGSubterraneanGate>();
			s.template registerType<CGMonolith, CGWhirlpool>();
	s.template registerType<CGObjectInstance, CGSignBottle>();
	s.template registerType<CGObjectInstance, CGKeys>();
		s.template registerType<CGKeys, CGKeymasterTent>();
		s.template registerType<CGKeys, CGBorderGuard>(); s.template registerType<IQuestObject, CGBorderGuard>();
			s.template registerType<CGBorderGuard, CGBorderGate>();
	s.template registerType<CGObjectInstance, CGBoat>();
	s.template registerType<CGObjectInstance, CGMagi>();
	s.template registerType<CGObjectInstance, CGSirens>();
	s.template registerType<CGObjectInstance, CGShipyard>();
	s.template registerType<CGObjectInstance, CGDenOfthieves>();
	s.template registerType<CGObjectInstance, CGLighthouse>();
	s.template registerType<CGObjectInstance, CGTerrainPatch>();
	s.template registerType<CGObjectInstance, HillFort>();
	s.template registerType<CGObjectInstance, CGMarket>();
		s.template registerType<CGMarket, CGBlackMarket>();
		s.template registerType<CGMarket, CGUniversity>();
	s.template registerType<CGObjectInstance, CGHeroPlaceholder>();

	s.template registerType<CGObjectInstance, CArmedInstance>(); s.template registerType<CBonusSystemNode, CArmedInstance>(); s.template registerType<CCreatureSet, CArmedInstance>();

	// Armed objects
	s.template registerType<CArmedInstance, CGHeroInstance>(); s.template registerType<CArtifactSet, CGHeroInstance>();
	s.template registerType<CArmedInstance, CGDwelling>();
		s.template registerType<CGDwelling, CGTownInstance>();
	s.template registerType<CArmedInstance, CGPandoraBox>();
		s.template registerType<CGPandoraBox, CGEvent>();
	s.template registerType<CArmedInstance, CGCreature>();
	s.template registerType<CArmedInstance, CGGarrison>();
	s.template registerType<CArmedInstance, CGArtifact>();
	s.template registerType<CArmedInstance, CGResource>();
	s.template registerType<CArmedInstance, CGMine>();
	s.template registerType<CArmedInstance, CBank>();
	s.template registerType<CArmedInstance, CGSeerHut>(); s.template registerType<IQuestObject, CGSeerHut>();
	s.template registerType<CGSeerHut, CGQuestGuard>();

	s.template registerType<IUpdater, GrowsWithLevelUpdater>();
	s.template registerType<IUpdater, TimesHeroLevelUpdater>();
	s.template registerType<IUpdater, TimesStackLevelUpdater>();
	s.template registerType<IUpdater, OwnerUpdater>();
	s.template registerType<IUpdater, ArmyMovementUpdater>();

	s.template registerType<ILimiter, AnyOfLimiter>();
	s.template registerType<ILimiter, NoneOfLimiter>();
	s.template registerType<ILimiter, OppositeSideLimiter>();
	//new types (other than netpacks) must register here
	//order of type registration is critical for loading old savegames

	//Other object-related
	s.template registerType<IObjectInterface, CGTownBuilding>();
		s.template registerType<CGTownBuilding, CTownBonus>();
		s.template registerType<CGTownBuilding, COPWBonus>();
		s.template registerType<CGTownBuilding, CTownRewardableBuilding>();

	s.template registerType<CGObjectInstance, CRewardableObject>();

	s.template registerType<CGObjectInstance, CTeamVisited>();
		s.template registerType<CTeamVisited, CGObelisk>();

	//end of objects

	//////////////////////////////////////////////////////////////////////////
	// Bonus system
	//////////////////////////////////////////////////////////////////////////
	//s.template registerType<IPropagator>();
	s.template registerType<IPropagator, CPropagatorNodeType>();

	// Limiters
	//s.template registerType<ILimiter>();
	s.template registerType<ILimiter, AllOfLimiter>();
	s.template registerType<ILimiter, CCreatureTypeLimiter>();
	s.template registerType<ILimiter, HasAnotherBonusLimiter>();
	s.template registerType<ILimiter, CreatureTerrainLimiter>();
	s.template registerType<ILimiter, FactionLimiter>();
	s.template registerType<ILimiter, CreatureLevelLimiter>();
	s.template registerType<ILimiter, CreatureAlignmentLimiter>();
	s.template registerType<ILimiter, RankRangeLimiter>();
	s.template registerType<ILimiter, UnitOnHexLimiter>();

//	s.template registerType<CBonusSystemNode>();
	s.template registerType<CBonusSystemNode, CArtifact>();
	s.template registerType<CBonusSystemNode, CCreature>();
	s.template registerType<CBonusSystemNode, CStackInstance>();
	s.template registerType<CStackInstance, CCommanderInstance>();
	s.template registerType<CBonusSystemNode, PlayerState>();
	s.template registerType<CBonusSystemNode, TeamState>();
	//s.template registerType<CGameState>(); //TODO
	//s.template registerType<CArmedInstance>();
	s.template registerType<CBonusSystemNode, CStack>();
	s.template registerType<CBonusSystemNode, BattleInfo>();
	//s.template registerType<QuestInfo>();
	s.template registerType<CBonusSystemNode, CArtifactInstance>();

	//s.template registerType<CObstacleInstance>();
		s.template registerType<CObstacleInstance, SpellCreatedObstacle>();

	s.template registerType<CGMarket, CGArtifactsAltar>();
}

VCMI_LIB_NAMESPACE_END
