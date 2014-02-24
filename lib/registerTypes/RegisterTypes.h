#pragma  once

#include "Connection.h"
#include "NetPacks.h"
#include "VCMI_Lib.h"
#include "CArtHandler.h"
#include "CObjectHandler.h"
#include "CGameState.h"
#include "CHeroHandler.h"
#include "CTownHandler.h"
#include "CModHandler.h" //needed?

/*
 * RegisterTypes.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

template<typename Serializer>
void registerTypesMapObjects(Serializer &s)
{
	//////////////////////////////////////////////////////////////////////////
	// Adventure map objects (and related)
	////////////////////////////////////////////////////////////////////////// 
	s.template registerType<IObjectInterface, CGObjectInstance>();

	// Non-armed objects
	s.template registerType<CGObjectInstance, CGTeleport>();
	s.template registerType<CGObjectInstance, CGPickable>();
	s.template registerType<CGObjectInstance, CGSignBottle>();
	s.template registerType<CGObjectInstance, CGScholar>();
	s.template registerType<CGObjectInstance, CGBonusingObject>();
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
		s.template registerType<CBank, CGPyramid>();
	s.template registerType<CArmedInstance, CGSeerHut>(); s.template registerType<IQuestObject, CGSeerHut>();
	s.template registerType<CGSeerHut, CGQuestGuard>();


	//Other object-related
	s.template registerType<IObjectInterface, CGTownBuilding>();
		s.template registerType<CGTownBuilding, CTownBonus>();
			s.template registerType<CGTownBuilding, COPWBonus>();


	s.template registerType<CGObjectInstance, CGVisitableOPH>();

	s.template registerType<CGObjectInstance, CGVisitableOPW>();
	s.template registerType<CGVisitableOPW, CGMagicSpring>();

	s.template registerType<CGObjectInstance, CPlayersVisited>();
		s.template registerType<CPlayersVisited, CGWitchHut>();
		s.template registerType<CPlayersVisited, CGShrine>();
		s.template registerType<CPlayersVisited, CGOnceVisitable>();
		s.template registerType<CPlayersVisited, CCartographer>();
		s.template registerType<CPlayersVisited, CGObelisk>();

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
	s.template registerType<ILimiter, LimiterList>();
	s.template registerType<ILimiter, CCreatureTypeLimiter>();
	s.template registerType<ILimiter, HasAnotherBonusLimiter>();
	s.template registerType<ILimiter, CreatureNativeTerrainLimiter>();
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
	s.template registerType<CPackForClient, YourTurn>();
	s.template registerType<CPackForClient, SetResource>();
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
	s.template registerType<CPackForClient, UpdateCampaignState>();
	s.template registerType<CPackForClient, PrepareForAdvancingCampaign>();
	s.template registerType<CPackForClient, UpdateArtHandlerLists>();
	s.template registerType<CPackForClient, UpdateMapEvents>();
	s.template registerType<CPackForClient, UpdateCastleEvents>();
	s.template registerType<CPackForClient, RemoveObject>();
	s.template registerType<CPackForClient, TryMoveHero>();
	//s.template registerType<CPackForClient, SetGarrisons>();
	s.template registerType<CPackForClient, NewStructures>();
	s.template registerType<CPackForClient, RazeStructures>();
	s.template registerType<CPackForClient, SetAvailableCreatures>();
	s.template registerType<CPackForClient, SetHeroesInTown>();
	//s.template registerType<CPackForClient, SetHeroArtifacts>();
	s.template registerType<CPackForClient, HeroRecruited>();
	s.template registerType<CPackForClient, GiveHero>();
	s.template registerType<CPackForClient, NewTurn>();
	s.template registerType<CPackForClient, InfoWindow>();
	s.template registerType<CPackForClient, SetObjectProperty>();
	s.template registerType<CPackForClient, SetHoverName>();
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
}

template<typename Serializer>
void registerTypesClientPacks2(Serializer &s)
{
	s.template registerType<CPackForClient, BattleStart>();
	s.template registerType<CPackForClient, BattleNextRound>();
	s.template registerType<CPackForClient, BattleSetActiveStack>();
	s.template registerType<CPackForClient, BattleResult>();
	s.template registerType<CPackForClient, BattleStackMoved>();
	s.template registerType<CPackForClient, BattleStackAttacked>();
	s.template registerType<CPackForClient, BattleAttack>();
	s.template registerType<CPackForClient, StartAction>();
	s.template registerType<CPackForClient, EndAction>();
	s.template registerType<CPackForClient, BattleSpellCast>();
	s.template registerType<CPackForClient, SetStackEffect>();
	s.template registerType<CPackForClient, BattleTriggerEffect>();
	s.template registerType<CPackForClient, BattleObstaclePlaced>();
	s.template registerType<CPackForClient, BattleSetStackProperty>();
	s.template registerType<CPackForClient, StacksInjured>();
	s.template registerType<CPackForClient, BattleResultsApplied>();
	s.template registerType<CPackForClient, StacksHealedOrResurrected>();
	s.template registerType<CPackForClient, ObstaclesRemoved>();
	s.template registerType<CPackForClient, CatapultAttack>();
	s.template registerType<CPackForClient, BattleStacksRemoved>();
	s.template registerType<CPackForClient, BattleStackAdded>();

	s.template registerType<CPackForClient, Query>();
	s.template registerType<Query, HeroLevelUp>();
	s.template registerType<Query, CommanderLevelUp>();
	s.template registerType<Query, BlockingDialog>();
	s.template registerType<Query, GarrisonDialog>();
	s.template registerType<Query, ExchangeDialog>();

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

	s.template registerType<CPackForClient, SaveGame>();
	s.template registerType<CPackForClient, SetSelection>();
	s.template registerType<CPackForClient, PlayerMessage>();
}

template<typename Serializer>
void registerTypesServerPacks(Serializer &s)
{
	s.template registerType<CPack, CPackForServer>();
	s.template registerType<CPackForServer, CloseServer>();
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
	s.template registerType<CPackForServer, CommitPackage>();

	s.template registerType<CPackForServer, SaveGame>();
	s.template registerType<CPackForServer, SetSelection>();
	s.template registerType<CPackForServer, PlayerMessage>();
}

template<typename Serializer>
void registerTypesPregamePacks(Serializer &s)
{
	s.template registerType<CPack, CPackForSelectionScreen>();
	s.template registerType<CPackForSelectionScreen, CPregamePackToPropagate>();
	s.template registerType<CPackForSelectionScreen, CPregamePackToHost>();

	s.template registerType<CPregamePackToPropagate, ChatMessage>();
	s.template registerType<CPregamePackToPropagate, QuitMenuWithoutStarting>();
	s.template registerType<CPregamePackToPropagate, SelectMap>();
	s.template registerType<CPregamePackToPropagate, UpdateStartOptions>();
	s.template registerType<CPregamePackToPropagate, PregameGuiAction>();
	s.template registerType<CPregamePackToPropagate, PlayerLeft>();
	s.template registerType<CPregamePackToPropagate, PlayersNames>();
	s.template registerType<CPregamePackToPropagate, StartWithCurrentSettings>();

	s.template registerType<CPregamePackToHost, PlayerJoined>();
	s.template registerType<CPregamePackToHost, RequestOptionsChange>();
}

template<typename Serializer>
void registerTypes(Serializer &s)
{
	registerTypesMapObjects(s);
	registerTypesClientPacks1(s);
	registerTypesClientPacks2(s);
	registerTypesServerPacks(s);
	registerTypesPregamePacks(s);
}

#ifndef INSTANTIATE_REGISTER_TYPES_HERE
extern template DLL_LINKAGE void registerTypes<CISer<CConnection>>(CISer<CConnection>& s);
extern template DLL_LINKAGE void registerTypes<COSer<CConnection>>(COSer<CConnection>& s);
extern template DLL_LINKAGE void registerTypes<CSaveFile>(CSaveFile & s);
extern template DLL_LINKAGE void registerTypes<CLoadFile>(CLoadFile & s);
extern template DLL_LINKAGE void registerTypes<CTypeList>(CTypeList & s);
extern template DLL_LINKAGE void registerTypes<CLoadIntegrityValidator>(CLoadIntegrityValidator & s);
extern template DLL_LINKAGE void registerTypes<CISer<CMemorySerializer>>(CISer<CMemorySerializer> & s);
extern template DLL_LINKAGE void registerTypes<COSer<CMemorySerializer>>(COSer<CMemorySerializer> & s);
#endif

