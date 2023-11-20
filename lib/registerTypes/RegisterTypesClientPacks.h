/*
 * RegisterTypesClientPacks.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../networkPacks/PacksForClient.h"
#include "../networkPacks/PacksForClientBattle.h"
#include "../networkPacks/SetStackEffect.h"

VCMI_LIB_NAMESPACE_BEGIN

template<typename Serializer>
void registerTypesClientPacks(Serializer &s)
{
	s.template registerType<CPack, CPackForClient>();

	s.template registerType<CPackForClient, PackageApplied>();
	s.template registerType<CPackForClient, SystemMessage>();
	s.template registerType<CPackForClient, PlayerBlocked>();
	s.template registerType<CPackForClient, PlayerCheated>();
	s.template registerType<CPackForClient, PlayerStartsTurn>();
	s.template registerType<CPackForClient, DaysWithoutTown>();
	s.template registerType<CPackForClient, TurnTimeUpdate>();
	s.template registerType<CPackForClient, SetResources>();
	s.template registerType<CPackForClient, SetPrimSkill>();
	s.template registerType<CPackForClient, SetSecSkill>();
	s.template registerType<CPackForClient, HeroVisitCastle>();
	s.template registerType<CPackForClient, ChangeSpells>();
	s.template registerType<CPackForClient, SetMana>();
	s.template registerType<CPackForClient, SetMovePoints>();
	s.template registerType<CPackForClient, FoWChange>();
	s.template registerType<CPackForClient, SetAvailableHero>();
	s.template registerType<CPackForClient, GiveBonus>();
	s.template registerType<CPackForClient, ChangeObjPos>();
	s.template registerType<CPackForClient, PlayerEndsTurn>();
	s.template registerType<CPackForClient, PlayerEndsGame>();
	s.template registerType<CPackForClient, PlayerReinitInterface>();
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
	s.template registerType<CPackForClient, EntitiesChanged>();
	s.template registerType<CPackForClient, BattleStart>();
	s.template registerType<CPackForClient, BattleNextRound>();
	s.template registerType<CPackForClient, BattleSetActiveStack>();
	s.template registerType<CPackForClient, BattleResult>();
	s.template registerType<CPackForClient, BattleResultAccepted>();
	s.template registerType<CPackForClient, BattleCancelled>();
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
	s.template registerType<CArtifactOperationPack, BulkMoveArtifacts>();

	s.template registerType<CPackForClient, PlayerMessageClient>();
	s.template registerType<CGarrisonOperationPack, BulkRebalanceStacks>();
	s.template registerType<CGarrisonOperationPack, BulkSmartRebalanceStacks>();
}

VCMI_LIB_NAMESPACE_END
