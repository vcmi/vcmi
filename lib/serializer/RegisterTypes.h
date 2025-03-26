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

#include "../CPlayerState.h"
#include "../CStack.h"
#include "../battle/BattleInfo.h"
#include "../battle/CObstacleInstance.h"
#include "../bonuses/Limiters.h"
#include "../bonuses/Propagators.h"
#include "../bonuses/Updaters.h"
#include "../campaign/CampaignState.h"
#include "../gameState/CGameState.h"
#include "../gameState/CGameStateCampaign.h"
#include "../gameState/TavernHeroesPool.h"

#include "../mapObjects/CGCreature.h"
#include "../mapObjects/CGDwelling.h"
#include "../mapObjects/CGResource.h"
#include "../mapObjects/CGMarket.h"
#include "../mapObjects/CGPandoraBox.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/CQuest.h"
#include "../mapObjects/FlaggableMapObject.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapObjects/TownBuildingInstance.h"

#include "../mapping/CMap.h"
#include "../networkPacks/PacksForClient.h"
#include "../networkPacks/PacksForClientBattle.h"
#include "../networkPacks/PacksForLobby.h"
#include "../networkPacks/PacksForServer.h"
#include "../networkPacks/SaveLocalState.h"
#include "../networkPacks/SetRewardableConfiguration.h"
#include "../networkPacks/SetStackEffect.h"

VCMI_LIB_NAMESPACE_BEGIN

/// This method defines all types that are part of Serializeable hieararchy and can be serialized as their base type
/// Each class is registered with a unique index that is used to determine correct type on deserialization
/// For example, if CGHeroInstance is serialized as pointer to CGObjectInstance serializer will write type index for CGHeroInstance, followed by CGHeroInstance::serialize() call
/// Similarly, on deserialize, game will look up type index of object that was serialized as this CGObjectInstance and will load it as CGHeroInstance instead
/// Meaning, these type indexes must NEVER change.
/// If type is removed please only remove corresponding type, without adjusting indexes of following types
/// NOTE: when removing type please consider potential save compatibility handling
/// Similarly, when adding new type make sure to add it to the very end of this list with new type index
template<typename Serializer>
void registerTypes(Serializer &s)
{
	static_assert(std::is_abstract_v<IObjectInterface>, "If this type is no longer abstract consider registering it for serialization with ID 1");
	static_assert(std::is_abstract_v<CGTeleport>, "If this type is no longer abstract consider registering it for serialization with ID 3");
	static_assert(std::is_abstract_v<IQuestObject>, "If this type is no longer abstract consider registering it for serialization with ID 11");
	static_assert(std::is_abstract_v<CArtifactSet>, "If this type is no longer abstract consider registering it for serialization with ID 29");
	static_assert(std::is_abstract_v<CPackForClient>, "If this type is no longer abstract consider registering it for serialization with ID 83");
	static_assert(std::is_abstract_v<Query>, "If this type is no longer abstract consider registering it for serialization with ID 153");
	static_assert(std::is_abstract_v<CGarrisonOperationPack>, "If this type is no longer abstract consider registering it for serialization with ID 161");
	static_assert(std::is_abstract_v<CArtifactOperationPack>, "If this type is no longer abstract consider registering it for serialization with ID 168");

	s.template registerType<CGObjectInstance>(2);
	s.template registerType<CGMonolith>(4);
	s.template registerType<CGSubterraneanGate>(5);
	s.template registerType<CGWhirlpool>(6);
	s.template registerType<CGSignBottle>(7);
	s.template registerType<CGKeys>(8);
	s.template registerType<CGKeymasterTent>(9);
	s.template registerType<CGBorderGuard>(10);
	s.template registerType<CGBorderGate>(12);
	s.template registerType<CGBoat>(13);
	s.template registerType<CGMagi>(14);
	s.template registerType<CGSirens>(15);
	s.template registerType<CGShipyard>(16);
	s.template registerType<CGDenOfthieves>(17);
	s.template registerType<FlaggableMapObject>(18);
	s.template registerType<CGTerrainPatch>(19);
	s.template registerType<HillFort>(20);
	s.template registerType<CGMarket>(21);
	s.template registerType<CGBlackMarket>(22);
	s.template registerType<CGUniversity>(23);
	s.template registerType<CGHeroPlaceholder>(24);
	s.template registerType<CArmedInstance>(25);
	s.template registerType<CBonusSystemNode>(26);
	s.template registerType<CCreatureSet>(27);
	s.template registerType<CGHeroInstance>(28);
	s.template registerType<CGDwelling>(30);
	s.template registerType<CGTownInstance>(31);
	s.template registerType<CGPandoraBox>(32);
	s.template registerType<CGEvent>(33);
	s.template registerType<CGCreature>(34);
	s.template registerType<CGGarrison>(35);
	s.template registerType<CGArtifact>(36);
	s.template registerType<CGResource>(37);
	s.template registerType<CGMine>(38);
	s.template registerType<CGSeerHut>(40);
	s.template registerType<CGQuestGuard>(41);
	s.template registerType<IUpdater>(42);
	s.template registerType<GrowsWithLevelUpdater>(43);
	s.template registerType<TimesHeroLevelUpdater>(44);
	s.template registerType<TimesStackLevelUpdater>(45);
	s.template registerType<OwnerUpdater>(46);
	s.template registerType<ArmyMovementUpdater>(47);
	s.template registerType<ILimiter>(48);
	s.template registerType<AnyOfLimiter>(49);
	s.template registerType<NoneOfLimiter>(50);
	s.template registerType<OppositeSideLimiter>(51);
	s.template registerType<TownBuildingInstance>(52);
	s.template registerType<TownRewardableBuildingInstance>(53);
	s.template registerType<CRewardableObject>(56);
	s.template registerType<CTeamVisited>(57);
	s.template registerType<CGObelisk>(58);
	s.template registerType<IPropagator>(59);
	s.template registerType<CPropagatorNodeType>(60);
	s.template registerType<AllOfLimiter>(61);
	s.template registerType<CCreatureTypeLimiter>(62);
	s.template registerType<HasAnotherBonusLimiter>(63);
	s.template registerType<CreatureTerrainLimiter>(64);
	s.template registerType<FactionLimiter>(65);
	s.template registerType<CreatureLevelLimiter>(66);
	s.template registerType<CreatureAlignmentLimiter>(67);
	s.template registerType<RankRangeLimiter>(68);
	s.template registerType<UnitOnHexLimiter>(69);
	s.template registerType<CArtifact>(70);
	s.template registerType<CCreature>(71);
	s.template registerType<CStackInstance>(72);
	s.template registerType<CCommanderInstance>(73);
	s.template registerType<PlayerState>(74);
	s.template registerType<TeamState>(75);
	s.template registerType<CStack>(76);
	s.template registerType<BattleInfo>(77);
	s.template registerType<CArtifactInstance>(78);
	s.template registerType<CObstacleInstance>(79);
	s.template registerType<SpellCreatedObstacle>(80);
	s.template registerType<CPack>(82);
	s.template registerType<PackageApplied>(84);
	s.template registerType<SystemMessage>(85);
	s.template registerType<PlayerBlocked>(86);
	s.template registerType<PlayerCheated>(87);
	s.template registerType<PlayerStartsTurn>(88);
	s.template registerType<DaysWithoutTown>(89);
	s.template registerType<TurnTimeUpdate>(90);
	s.template registerType<SetResources>(91);
	s.template registerType<SetPrimSkill>(92);
	s.template registerType<SetSecSkill>(93);
	s.template registerType<HeroVisitCastle>(94);
	s.template registerType<ChangeSpells>(95);
	s.template registerType<SetMana>(96);
	s.template registerType<SetMovePoints>(97);
	s.template registerType<FoWChange>(98);
	s.template registerType<SetAvailableHero>(99);
	s.template registerType<GiveBonus>(100);
	s.template registerType<ChangeObjPos>(101);
	s.template registerType<PlayerEndsTurn>(102);
	s.template registerType<PlayerEndsGame>(103);
	s.template registerType<PlayerReinitInterface>(104);
	s.template registerType<RemoveBonus>(105);
	s.template registerType<UpdateArtHandlerLists>(106);
	s.template registerType<ChangeFormation>(107);
	s.template registerType<RemoveObject>(108);
	s.template registerType<TryMoveHero>(109);
	s.template registerType<NewStructures>(110);
	s.template registerType<RazeStructures>(111);
	s.template registerType<SetAvailableCreatures>(112);
	s.template registerType<SetHeroesInTown>(113);
	s.template registerType<HeroRecruited>(114);
	s.template registerType<GiveHero>(115);
	s.template registerType<NewTurn>(116);
	s.template registerType<InfoWindow>(117);
	s.template registerType<SetObjectProperty>(118);
	s.template registerType<AdvmapSpellCast>(119);
	s.template registerType<OpenWindow>(120);
	s.template registerType<NewObject>(121);
	s.template registerType<NewArtifact>(122);
	s.template registerType<AddQuest>(123);
	s.template registerType<SetAvailableArtifacts>(124);
	s.template registerType<CenterView>(125);
	s.template registerType<HeroVisit>(126);
	s.template registerType<SetCommanderProperty>(127);
	s.template registerType<ChangeObjectVisitors>(128);
	s.template registerType<ChangeArtifactsCostume>(129);
	s.template registerType<ShowWorldViewEx>(130);
	s.template registerType<EntitiesChanged>(131);
	s.template registerType<BattleStart>(132);
	s.template registerType<BattleNextRound>(133);
	s.template registerType<BattleSetActiveStack>(134);
	s.template registerType<BattleResult>(135);
	s.template registerType<BattleResultAccepted>(136);
	s.template registerType<BattleCancelled>(137);
	s.template registerType<BattleLogMessage>(138);
	s.template registerType<BattleStackMoved>(139);
	s.template registerType<BattleAttack>(140);
	s.template registerType<StartAction>(141);
	s.template registerType<EndAction>(142);
	s.template registerType<BattleSpellCast>(143);
	s.template registerType<SetStackEffect>(144);
	s.template registerType<BattleTriggerEffect>(145);
	s.template registerType<BattleUpdateGateState>(146);
	s.template registerType<BattleSetStackProperty>(147);
	s.template registerType<StacksInjured>(148);
	s.template registerType<BattleResultsApplied>(149);
	s.template registerType<BattleUnitsChanged>(150);
	s.template registerType<BattleObstaclesChanged>(151);
	s.template registerType<CatapultAttack>(152);
	s.template registerType<HeroLevelUp>(154);
	s.template registerType<CommanderLevelUp>(155);
	s.template registerType<BlockingDialog>(156);
	s.template registerType<GarrisonDialog>(157);
	s.template registerType<ExchangeDialog>(158);
	s.template registerType<TeleportDialog>(159);
	s.template registerType<MapObjectSelectDialog>(160);
	s.template registerType<ChangeStackCount>(162);
	s.template registerType<SetStackType>(163);
	s.template registerType<EraseStack>(164);
	s.template registerType<SwapStacks>(165);
	s.template registerType<InsertNewStack>(166);
	s.template registerType<RebalanceStacks>(167);
	s.template registerType<PutArtifact>(169);
	s.template registerType<BulkEraseArtifacts>(170);
	s.template registerType<AssembledArtifact>(171);
	s.template registerType<DisassembledArtifact>(172);
	s.template registerType<BulkMoveArtifacts>(173);
	s.template registerType<PlayerMessageClient>(174);
	s.template registerType<BulkRebalanceStacks>(175);
	s.template registerType<BulkSmartRebalanceStacks>(176);
	s.template registerType<SetRewardableConfiguration>(177);
	s.template registerType<CPackForServer>(179);
	s.template registerType<EndTurn>(180);
	s.template registerType<DismissHero>(181);
	s.template registerType<MoveHero>(182);
	s.template registerType<ArrangeStacks>(183);
	s.template registerType<DisbandCreature>(184);
	s.template registerType<BuildStructure>(185);
	s.template registerType<VisitTownBuilding>(186);
	s.template registerType<RecruitCreatures>(187);
	s.template registerType<UpgradeCreature>(188);
	s.template registerType<GarrisonHeroSwap>(189);
	s.template registerType<ExchangeArtifacts>(190);
	s.template registerType<AssembleArtifacts>(191);
	s.template registerType<BuyArtifact>(192);
	s.template registerType<TradeOnMarketplace>(193);
	s.template registerType<SetFormation>(194);
	s.template registerType<HireHero>(195);
	s.template registerType<BuildBoat>(196);
	s.template registerType<QueryReply>(197);
	s.template registerType<MakeAction>(198);
	s.template registerType<DigWithHero>(199);
	s.template registerType<CastAdvSpell>(200);
	s.template registerType<CastleTeleportHero>(201);
	s.template registerType<SaveGame>(202);
	s.template registerType<PlayerMessage>(203);
	s.template registerType<BulkSplitStack>(204);
	s.template registerType<BulkMergeStacks>(205);
	s.template registerType<BulkSmartSplitStack>(206);
	s.template registerType<BulkMoveArmy>(207);
	s.template registerType<BulkExchangeArtifacts>(208);
	s.template registerType<ManageBackpackArtifacts>(209);
	s.template registerType<ManageEquippedArtifacts>(210);
	s.template registerType<EraseArtifactByClient>(211);
	s.template registerType<GamePause>(212);
	s.template registerType<CPackForLobby>(213);
	s.template registerType<CLobbyPackToPropagate>(214);
	s.template registerType<CLobbyPackToServer>(215);
	s.template registerType<LobbyClientConnected>(216);
	s.template registerType<LobbyClientDisconnected>(217);
	s.template registerType<LobbyChatMessage>(218);
	s.template registerType<LobbyPvPAction>(219);
	s.template registerType<LobbyGuiAction>(220);
	s.template registerType<LobbyLoadProgress>(221);
	s.template registerType<LobbyRestartGame>(222);
	s.template registerType<LobbyPrepareStartGame>(223);
	s.template registerType<LobbyStartGame>(224);
	s.template registerType<LobbyChangeHost>(225);
	s.template registerType<LobbyUpdateState>(226);
	s.template registerType<LobbyShowMessage>(227);
	s.template registerType<LobbyChangePlayerOption>(228);
	s.template registerType<LobbySetMap>(229);
	s.template registerType<LobbySetCampaign>(230);
	s.template registerType<LobbySetCampaignMap>(231);
	s.template registerType<LobbySetCampaignBonus>(232);
	s.template registerType<LobbySetPlayer>(233);
	s.template registerType<LobbySetPlayerName>(234);
	s.template registerType<LobbySetPlayerHandicap>(235);
	s.template registerType<LobbySetTurnTime>(236);
	s.template registerType<LobbySetSimturns>(237);
	s.template registerType<LobbySetDifficulty>(238);
	s.template registerType<LobbyForceSetPlayer>(239);
	s.template registerType<LobbySetExtraOptions>(240);
	s.template registerType<SpellResearch>(241);
	s.template registerType<SetResearchedSpells>(242);
	s.template registerType<SaveLocalState>(243);
	s.template registerType<LobbyDelete>(244);
}

VCMI_LIB_NAMESPACE_END
