/*
 * RegisterTypes.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../NetPacks.h"
#include "../NetPacksLobby.h"
#include "../VCMI_Lib.h"
#include "../CArtHandler.h"
#include "../CPlayerState.h"
#include "../CHeroHandler.h"
#include "../CTownHandler.h"
#include "../CModHandler.h" //needed?
#include "../mapObjects/CObjectClassesHandler.h"
#include "../mapObjects/CRewardableConstructor.h"
#include "../mapObjects/CommonConstructors.h"
#include "../mapObjects/MapObjects.h"
#include "../battle/CObstacleInstance.h"
#include "../CStack.h"

VCMI_LIB_NAMESPACE_BEGIN

class BinarySerializer;
class BinaryDeserializer;
class CTypeList;

template<typename Serializer>
void registerTypesMapObjects1(Serializer &s)
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
	s.template registerType<CGObjectInstance, CGScholar>();
	s.template registerType<CGObjectInstance, CGMagicWell>();
	s.template registerType<CGObjectInstance, CGObservatory>();
	s.template registerType<CGObjectInstance, CGKeys>();
		s.template registerType<CGKeys, CGKeymasterTent>();
		s.template registerType<CGKeys, CGBorderGuard>(); s.template registerType<IQuestObject, CGBorderGuard>();
			s.template registerType<CGBorderGuard, CGBorderGate>();
	s.template registerType<CGObjectInstance, CGBoat>();
	s.template registerType<CGObjectInstance, CGMagi>();
	s.template registerType<CGObjectInstance, CGSirens>();
	s.template registerType<CGObjectInstance, CGShipyard>(); s.template registerType<IShipyard, CGShipyard>();
	s.template registerType<CGObjectInstance, CGDenOfthieves>();
	s.template registerType<CGObjectInstance, CGLighthouse>();
	s.template registerType<CGObjectInstance, CGTerrainPatch>();
	s.template registerType<CGObjectInstance, CGMarket>(); s.template registerType<IMarket, CGMarket>();
		s.template registerType<CGMarket, CGBlackMarket>();
		s.template registerType<CGMarket, CGUniversity>();
	s.template registerType<CGObjectInstance, CGHeroPlaceholder>();

	s.template registerType<CGObjectInstance, CArmedInstance>(); s.template registerType<CBonusSystemNode, CArmedInstance>(); s.template registerType<CCreatureSet, CArmedInstance>();

	// Armed objects
	s.template registerType<CArmedInstance, CGHeroInstance>(); s.template registerType<IBoatGenerator, CGHeroInstance>(); s.template registerType<CArtifactSet, CGHeroInstance>();
	s.template registerType<CArmedInstance, CGDwelling>();
		s.template registerType<CGDwelling, CGTownInstance>(); s.template registerType<IShipyard, CGTownInstance>(); s.template registerType<IMarket, CGTownInstance>();
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
}

template<typename Serializer>
void registerTypesMapObjectTypes(Serializer &s)
{
	s.template registerType<AObjectTypeHandler, CRewardableConstructor>();
	s.template registerType<AObjectTypeHandler, CHeroInstanceConstructor>();
	s.template registerType<AObjectTypeHandler, CTownInstanceConstructor>();
	s.template registerType<AObjectTypeHandler, CDwellingInstanceConstructor>();
	s.template registerType<AObjectTypeHandler, CBankInstanceConstructor>();
	s.template registerType<AObjectTypeHandler, CObstacleConstructor>();

#define REGISTER_GENERIC_HANDLER(TYPENAME) s.template registerType<AObjectTypeHandler, CDefaultObjectTypeHandler<TYPENAME> >()

	REGISTER_GENERIC_HANDLER(CGObjectInstance);
	REGISTER_GENERIC_HANDLER(CGMarket);
	REGISTER_GENERIC_HANDLER(CCartographer);
	REGISTER_GENERIC_HANDLER(CGArtifact);
	REGISTER_GENERIC_HANDLER(CGBlackMarket);
	REGISTER_GENERIC_HANDLER(CGBoat);
	REGISTER_GENERIC_HANDLER(CGBonusingObject);
	REGISTER_GENERIC_HANDLER(CGBorderGate);
	REGISTER_GENERIC_HANDLER(CGBorderGuard);
	REGISTER_GENERIC_HANDLER(CGCreature);
	REGISTER_GENERIC_HANDLER(CGDenOfthieves);
	REGISTER_GENERIC_HANDLER(CGDwelling);
	REGISTER_GENERIC_HANDLER(CGEvent);
	REGISTER_GENERIC_HANDLER(CGGarrison);
	REGISTER_GENERIC_HANDLER(CGHeroPlaceholder);
	REGISTER_GENERIC_HANDLER(CGHeroInstance);
	REGISTER_GENERIC_HANDLER(CGKeymasterTent);
	REGISTER_GENERIC_HANDLER(CGLighthouse);
	REGISTER_GENERIC_HANDLER(CGTerrainPatch);
	REGISTER_GENERIC_HANDLER(CGMagi);
	REGISTER_GENERIC_HANDLER(CGMagicSpring);
	REGISTER_GENERIC_HANDLER(CGMagicWell);
	REGISTER_GENERIC_HANDLER(CGMarket);
	REGISTER_GENERIC_HANDLER(CGMine);
	REGISTER_GENERIC_HANDLER(CGObelisk);
	REGISTER_GENERIC_HANDLER(CGObservatory);
	REGISTER_GENERIC_HANDLER(CGOnceVisitable);
	REGISTER_GENERIC_HANDLER(CGPandoraBox);
	REGISTER_GENERIC_HANDLER(CGPickable);
	REGISTER_GENERIC_HANDLER(CGQuestGuard);
	REGISTER_GENERIC_HANDLER(CGResource);
	REGISTER_GENERIC_HANDLER(CGScholar);
	REGISTER_GENERIC_HANDLER(CGSeerHut);
	REGISTER_GENERIC_HANDLER(CGShipyard);
	REGISTER_GENERIC_HANDLER(CGShrine);
	REGISTER_GENERIC_HANDLER(CGSignBottle);
	REGISTER_GENERIC_HANDLER(CGSirens);
	REGISTER_GENERIC_HANDLER(CGMonolith);
	REGISTER_GENERIC_HANDLER(CGSubterraneanGate);
	REGISTER_GENERIC_HANDLER(CGWhirlpool);
	REGISTER_GENERIC_HANDLER(CGTownInstance);
	REGISTER_GENERIC_HANDLER(CGUniversity);
	REGISTER_GENERIC_HANDLER(CGVisitableOPH);
	REGISTER_GENERIC_HANDLER(CGVisitableOPW);
	REGISTER_GENERIC_HANDLER(CGWitchHut);

#undef REGISTER_GENERIC_HANDLER

	s.template registerType<IUpdater, GrowsWithLevelUpdater>();
	s.template registerType<IUpdater, TimesHeroLevelUpdater>();
	s.template registerType<IUpdater, TimesStackLevelUpdater>();
	s.template registerType<IUpdater, OwnerUpdater>();

	s.template registerType<ILimiter, AnyOfLimiter>();
	s.template registerType<ILimiter, NoneOfLimiter>();
	s.template registerType<ILimiter, OppositeSideLimiter>();
	//new types (other than netpacks) must register here
	//order of type registration is critical for loading old savegames
}

template<typename Serializer>
void registerTypesMapObjects2(Serializer &s)
{
	//Other object-related
	s.template registerType<IObjectInterface, CGTownBuilding>();
		s.template registerType<CGTownBuilding, CTownBonus>();
			s.template registerType<CGTownBuilding, COPWBonus>();

	s.template registerType<CGObjectInstance, CRewardableObject>();
		s.template registerType<CRewardableObject, CGPickable>();
		s.template registerType<CRewardableObject, CGBonusingObject>();
		s.template registerType<CRewardableObject, CGVisitableOPH>();
		s.template registerType<CRewardableObject, CGVisitableOPW>();
		s.template registerType<CRewardableObject, CGOnceVisitable>();
			s.template registerType<CGVisitableOPW, CGMagicSpring>();

	s.template registerType<CGObjectInstance, CTeamVisited>();
		s.template registerType<CTeamVisited, CGWitchHut>();
		s.template registerType<CTeamVisited, CGShrine>();
		s.template registerType<CTeamVisited, CCartographer>();
		s.template registerType<CTeamVisited, CGObelisk>();

	//s.template registerType<CQuest>();
	//s.template registerType<IQuestObject>();

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
	s.template registerType<ILimiter, CreatureFactionLimiter>();
	s.template registerType<ILimiter, CreatureAlignmentLimiter>();
	s.template registerType<ILimiter, RankRangeLimiter>();
	s.template registerType<ILimiter, StackOwnerLimiter>();

//	s.template registerType<CBonusSystemNode>();
	s.template registerType<CBonusSystemNode, CArtifact>();
	s.template registerType<CArtifact, CGrowingArtifact>();
	s.template registerType<CBonusSystemNode, CCreature>();
	s.template registerType<CBonusSystemNode, CStackInstance>();
	s.template registerType<CStackInstance, CCommanderInstance>();
	s.template registerType<CBonusSystemNode, PlayerState>();
	s.template registerType<CBonusSystemNode, TeamState>();
	//s.template registerType<CGameState>(); //TODO
	s.template registerType<CBonusSystemNode, CGHeroInstance::HeroSpecial>();
	//s.template registerType<CArmedInstance>();
	s.template registerType<CBonusSystemNode, CStack>();
	s.template registerType<CBonusSystemNode, BattleInfo>();
	//s.template registerType<QuestInfo>();
	s.template registerType<CBonusSystemNode, CArtifactInstance>();
	s.template registerType<CArtifactInstance, CCombinedArtifactInstance>();

	//s.template registerType<CObstacleInstance>();
		s.template registerType<CObstacleInstance, MoatObstacle>();
		s.template registerType<CObstacleInstance, SpellCreatedObstacle>();
}
template<typename Serializer>
void registerTypesClientPacks1(Serializer &s)
{
	s.template registerType<CPack, CPackForClient>();

	s.template registerType<CPackForClient, PackageApplied>();
	s.template registerType<CPackForClient, SystemMessage>();
	s.template registerType<CPackForClient, PlayerBlocked>();
	s.template registerType<CPackForClient, PlayerCheated>();
	s.template registerType<CPackForClient, YourTurn>();
	s.template registerType<CPackForClient, SetResources>();
	s.template registerType<CPackForClient, SetPrimSkill>();
	s.template registerType<CPackForClient, SetSecSkill>();
	s.template registerType<CPackForClient, HeroVisitCastle>();
	s.template registerType<CPackForClient, ChangeSpells>();
	s.template registerType<CPackForClient, SetMana>();
	s.template registerType<CPackForClient, SetMovePoints>();
	s.template registerType<CPackForClient, FoWChange>();
	s.template registerType<CPackForClient, SetAvailableHeroes>();
	s.template registerType<CPackForClient, GiveBonus>();
	s.template registerType<CPackForClient, ChangeObjPos>();
	s.template registerType<CPackForClient, PlayerEndsGame>();
	s.template registerType<CPackForClient, RemoveBonus>();
	s.template registerType<CPackForClient, UpdateArtHandlerLists>();
	s.template registerType<CPackForClient, UpdateMapEvents>();
	s.template registerType<CPackForClient, UpdateCastleEvents>();
	s.template registerType<CPackForClient, ChangeFormation>();
	s.template registerType<CPackForClient, RemoveObject>();
	s.template registerType<CPackForClient, TryMoveHero>();
	s.template registerType<CPackForClient, NewStructures>();
	s.template registerType<CPackForClient, RazeStructures>();
	s.template registerType<CPackForClient, SetAvailableCreatures>();
	s.template registerType<CPackForClient, SetHeroesInTown>();
	s.template registerType<CPackForClient, HeroRecruited>();
	s.template registerType<CPackForClient, GiveHero>();
	s.template registerType<CPackForClient, NewTurn>();
	s.template registerType<CPackForClient, InfoWindow>();
	s.template registerType<CPackForClient, SetObjectProperty>();
	s.template registerType<CPackForClient, ShowInInfobox>();
	s.template registerType<CPackForClient, AdvmapSpellCast>();
	s.template registerType<CPackForClient, OpenWindow>();
	s.template registerType<CPackForClient, NewObject>();
	s.template registerType<CPackForClient, NewArtifact>();
	s.template registerType<CPackForClient, AddQuest>();
	s.template registerType<CPackForClient, SetAvailableArtifacts>();
	s.template registerType<CPackForClient, CenterView>();
	s.template registerType<CPackForClient, HeroVisit>();
	s.template registerType<CPackForClient, SetCommanderProperty>();
	s.template registerType<CPackForClient, ChangeObjectVisitors>();
	s.template registerType<CPackForClient, ShowWorldViewEx>();
	s.template registerType<CPackForClient, PrepareHeroLevelUp>();
	s.template registerType<CPackForClient, EntitiesChanged>();
}

template<typename Serializer>
void registerTypesClientPacks2(Serializer &s)
{
	s.template registerType<CPackForClient, BattleStart>();
	s.template registerType<CPackForClient, BattleNextRound>();
	s.template registerType<CPackForClient, BattleSetActiveStack>();
	s.template registerType<CPackForClient, BattleResult>();
	s.template registerType<CPackForClient, BattleLogMessage>();
	s.template registerType<CPackForClient, BattleStackMoved>();
	s.template registerType<CPackForClient, BattleAttack>();
	s.template registerType<CPackForClient, StartAction>();
	s.template registerType<CPackForClient, EndAction>();
	s.template registerType<CPackForClient, BattleSpellCast>();
	s.template registerType<CPackForClient, SetStackEffect>();
	s.template registerType<CPackForClient, BattleTriggerEffect>();
	s.template registerType<CPackForClient, BattleUpdateGateState>();
	s.template registerType<CPackForClient, BattleSetStackProperty>();
	s.template registerType<CPackForClient, StacksInjured>();
	s.template registerType<CPackForClient, BattleResultsApplied>();
	s.template registerType<CPackForClient, BattleUnitsChanged>();
	s.template registerType<CPackForClient, BattleObstaclesChanged>();
	s.template registerType<CPackForClient, CatapultAttack>();

	s.template registerType<CPackForClient, Query>();
	s.template registerType<Query, HeroLevelUp>();
	s.template registerType<Query, CommanderLevelUp>();
	s.template registerType<Query, BlockingDialog>();
	s.template registerType<Query, GarrisonDialog>();
	s.template registerType<Query, ExchangeDialog>();
	s.template registerType<Query, TeleportDialog>();
	s.template registerType<Query, MapObjectSelectDialog>();

	s.template registerType<CPackForClient, CGarrisonOperationPack>();
	s.template registerType<CGarrisonOperationPack, ChangeStackCount>();
	s.template registerType<CGarrisonOperationPack, SetStackType>();
	s.template registerType<CGarrisonOperationPack, EraseStack>();
	s.template registerType<CGarrisonOperationPack, SwapStacks>();
	s.template registerType<CGarrisonOperationPack, InsertNewStack>();
	s.template registerType<CGarrisonOperationPack, RebalanceStacks>();

	s.template registerType<CPackForClient, CArtifactOperationPack>();
	s.template registerType<CArtifactOperationPack, PutArtifact>();
	s.template registerType<CArtifactOperationPack, EraseArtifact>();
	s.template registerType<CArtifactOperationPack, MoveArtifact>();
	s.template registerType<CArtifactOperationPack, AssembledArtifact>();
	s.template registerType<CArtifactOperationPack, DisassembledArtifact>();

	s.template registerType<CPackForClient, SaveGameClient>();
	s.template registerType<CPackForClient, PlayerMessageClient>();
	s.template registerType<CGarrisonOperationPack, BulkRebalanceStacks>();
	s.template registerType<CGarrisonOperationPack, BulkSmartRebalanceStacks>();
}

template<typename Serializer>
void registerTypesServerPacks(Serializer &s)
{
	s.template registerType<CPack, CPackForServer>();
	s.template registerType<CPackForServer, EndTurn>();
	s.template registerType<CPackForServer, DismissHero>();
	s.template registerType<CPackForServer, MoveHero>();
	s.template registerType<CPackForServer, ArrangeStacks>();
	s.template registerType<CPackForServer, DisbandCreature>();
	s.template registerType<CPackForServer, BuildStructure>();
	s.template registerType<CPackForServer, RecruitCreatures>();
	s.template registerType<CPackForServer, UpgradeCreature>();
	s.template registerType<CPackForServer, GarrisonHeroSwap>();
	s.template registerType<CPackForServer, ExchangeArtifacts>();
	s.template registerType<CPackForServer, AssembleArtifacts>();
	s.template registerType<CPackForServer, BuyArtifact>();
	s.template registerType<CPackForServer, TradeOnMarketplace>();
	s.template registerType<CPackForServer, SetFormation>();
	s.template registerType<CPackForServer, HireHero>();
	s.template registerType<CPackForServer, BuildBoat>();
	s.template registerType<CPackForServer, QueryReply>();
	s.template registerType<CPackForServer, MakeAction>();
	s.template registerType<CPackForServer, MakeCustomAction>();
	s.template registerType<CPackForServer, DigWithHero>();
	s.template registerType<CPackForServer, CastAdvSpell>();
	s.template registerType<CPackForServer, CastleTeleportHero>();
	s.template registerType<CPackForServer, SaveGame>();
	s.template registerType<CPackForServer, PlayerMessage>();
	s.template registerType<CPackForServer, BulkSplitStack>();
	s.template registerType<CPackForServer, BulkMergeStacks>();
	s.template registerType<CPackForServer, BulkSmartSplitStack>();
	s.template registerType<CPackForServer, BulkMoveArmy>();
}

template<typename Serializer>
void registerTypesLobbyPacks(Serializer &s)
{
	s.template registerType<CPack, CPackForLobby>();
	s.template registerType<CPackForLobby, CLobbyPackToPropagate>();
	s.template registerType<CPackForLobby, CLobbyPackToServer>();

	// Any client can sent
	s.template registerType<CLobbyPackToPropagate, LobbyClientConnected>();
	s.template registerType<CLobbyPackToPropagate, LobbyClientDisconnected>();
	s.template registerType<CLobbyPackToPropagate, LobbyChatMessage>();
	// Only host client send
	s.template registerType<CLobbyPackToPropagate, LobbyGuiAction>();
	s.template registerType<CLobbyPackToPropagate, LobbyStartGame>();
	s.template registerType<CLobbyPackToPropagate, LobbyChangeHost>();
	// Only server send
	s.template registerType<CLobbyPackToPropagate, LobbyUpdateState>();

	// For client with permissions
	s.template registerType<CLobbyPackToServer, LobbyChangePlayerOption>();
	// Only for host client
	s.template registerType<CLobbyPackToServer, LobbySetMap>();
	s.template registerType<CLobbyPackToServer, LobbySetCampaign>();
	s.template registerType<CLobbyPackToServer, LobbySetCampaignMap>();
	s.template registerType<CLobbyPackToServer, LobbySetCampaignBonus>();
	s.template registerType<CLobbyPackToServer, LobbySetPlayer>();
	s.template registerType<CLobbyPackToServer, LobbySetTurnTime>();
	s.template registerType<CLobbyPackToServer, LobbySetDifficulty>();
	s.template registerType<CLobbyPackToServer, LobbyForceSetPlayer>();
}

template<typename Serializer>
void registerTypes(Serializer &s)
{
	registerTypesMapObjects1(s);
	registerTypesMapObjects2(s);
	registerTypesMapObjectTypes(s);
	registerTypesClientPacks1(s);
	registerTypesClientPacks2(s);
	registerTypesServerPacks(s);
	registerTypesLobbyPacks(s);
}

#ifndef INSTANTIATE_REGISTER_TYPES_HERE

extern template DLL_LINKAGE void registerTypes<BinaryDeserializer>(BinaryDeserializer & s);
extern template DLL_LINKAGE void registerTypes<BinarySerializer>(BinarySerializer & s);
extern template DLL_LINKAGE void registerTypes<CTypeList>(CTypeList & s);

#endif


VCMI_LIB_NAMESPACE_END
