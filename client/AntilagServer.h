/*
 * AntilagServer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/networkPacks/NetPackVisitor.h"
#include "../server/IGameServer.h"
#include "../lib/network/NetworkInterface.h"
#include "../lib/serializer/IGameConnection.h"
#include "../lib/serializer/CSerializer.h"
#include "../lib/serializer/BinarySerializer.h"

VCMI_LIB_NAMESPACE_BEGIN
struct CPackForServer;
class IGameConnection;
VCMI_LIB_NAMESPACE_END

class CGameHandler;

class ConnectionPackWriter final : public IBinaryWriter
{
public:
	std::vector<std::byte> buffer;

	int write(const std::byte * data, unsigned size) final;
};

class AntilagFakeConnection final : public IGameConnection
{
public:
	void sendPack(const CPack & pack) override;
	std::unique_ptr<CPack> retrievePack(const std::vector<std::byte> & data) override;
	int getConnectionID() const override;

	PlayerColor senderID;
	uint32_t requestID;
	std::vector<ConnectionPackWriter> writtenPacks;
};

class AntilagReplyPredictionVisitor final : public VCMI_LIB_WRAP_NAMESPACE(ICPackVisitor)
{
	bool canBeAppliedValue = false;

	//void visitSaveGame(SaveGame & pack) override;
	//void visitGamePause(GamePause & pack) override;
	//void visitEndTurn(EndTurn & pack) override;
	//void visitDismissHero(DismissHero & pack) override;
	void visitMoveHero(MoveHero & pack) override;
	//void visitCastleTeleportHero(CastleTeleportHero & pack) override;
	//void visitArrangeStacks(ArrangeStacks & pack) override;
	//void visitBulkMoveArmy(BulkMoveArmy & pack) override;
	//void visitBulkSplitStack(BulkSplitStack & pack) override;
	//void visitBulkMergeStacks(BulkMergeStacks & pack) override;
	//void visitBulkSplitAndRebalanceStack(BulkSplitAndRebalanceStack & pack) override;
	//void visitDisbandCreature(DisbandCreature & pack) override;
	//void visitBuildStructure(BuildStructure & pack) override;
	//void visitSpellResearch(SpellResearch & pack) override;
	//void visitVisitTownBuilding(VisitTownBuilding & pack) override;
	//void visitRecruitCreatures(RecruitCreatures & pack) override;
	//void visitUpgradeCreature(UpgradeCreature & pack) override;
	//void visitGarrisonHeroSwap(GarrisonHeroSwap & pack) override;
	//void visitExchangeArtifacts(ExchangeArtifacts & pack) override;
	//void visitBulkExchangeArtifacts(BulkExchangeArtifacts & pack) override;
	//void visitManageBackpackArtifacts(ManageBackpackArtifacts & pack) override;
	//void visitManageEquippedArtifacts(ManageEquippedArtifacts & pack) override;
	//void visitAssembleArtifacts(AssembleArtifacts & pack) override;
	//void visitEraseArtifactByClient(EraseArtifactByClient & pack) override;
	//void visitBuyArtifact(BuyArtifact & pack) override;
	//void visitTradeOnMarketplace(TradeOnMarketplace & pack) override;
	//void visitSetFormation(SetFormation & pack) override;
	//void visitHireHero(HireHero & pack) override;
	//void visitBuildBoat(BuildBoat & pack) override;
	//void visitQueryReply(QueryReply & pack) override;
	//void visitMakeAction(MakeAction & pack) override;
	//void visitDigWithHero(DigWithHero & pack) override;
	//void visitCastAdvSpell(CastAdvSpell & pack) override;
	//void visitPlayerMessage(PlayerMessage & pack) override;
	//void visitSaveLocalState(SaveLocalState & pack) override;

public:
	AntilagReplyPredictionVisitor();

	bool canBeApplied() const;
};

class AntilagRollbackGeneratorVisitor final : public ICPackVisitor
{
private:
	const CGameState & gs;
	std::vector<std::unique_ptr<CPackForClient>> rollbackPacks;
	bool success = false;

	//void visitSetResources(SetResources & pack) override;
	//void visitSetPrimarySkill(SetPrimarySkill & pack) override;
	//void visitSetHeroExperience(SetHeroExperience & pack) override;
	//void visitGiveStackExperience(GiveStackExperience & pack) override;
	//void visitSetSecSkill(SetSecSkill & pack) override;
	//void visitHeroVisitCastle(HeroVisitCastle & pack) override;
	//void visitSetMana(SetMana & pack) override;
	//void visitSetMovePoints(SetMovePoints & pack) override;
	//void visitSetResearchedSpells(SetResearchedSpells & pack) override;
	//void visitFoWChange(FoWChange & pack) override;
	//void visitChangeStackCount(ChangeStackCount & pack) override;
	//void visitSetStackType(SetStackType & pack) override;
	//void visitEraseStack(EraseStack & pack) override;
	//void visitSwapStacks(SwapStacks & pack) override;
	//void visitInsertNewStack(InsertNewStack & pack) override;
	//void visitRebalanceStacks(RebalanceStacks & pack) override;
	//void visitBulkRebalanceStacks(BulkRebalanceStacks & pack) override;
	//void visitGrowUpArtifact(GrowUpArtifact & pack) override;
	//void visitPutArtifact(PutArtifact & pack) override;
	//void visitBulkEraseArtifacts(BulkEraseArtifacts & pack) override;
	//void visitBulkMoveArtifacts(BulkMoveArtifacts & pack) override;
	//void visitAssembledArtifact(AssembledArtifact & pack) override;
	//void visitDisassembledArtifact(DisassembledArtifact & pack) override;
	//void visitDischargeArtifact(DischargeArtifact & pack) override;
	//void visitHeroVisit(HeroVisit & pack) override;
	//void visitNewTurn(NewTurn & pack) override;
	//void visitGiveBonus(GiveBonus & pack) override;
	//void visitChangeObjPos(ChangeObjPos & pack) override;
	//void visitPlayerEndsTurn(PlayerEndsTurn & pack) override;
	//void visitPlayerEndsGame(PlayerEndsGame & pack) override;
	//void visitPlayerReinitInterface(PlayerReinitInterface & pack) override;
	//void visitRemoveBonus(RemoveBonus & pack) override;
	//void visitRemoveObject(RemoveObject & pack) override;
	void visitTryMoveHero(TryMoveHero & pack) override;
	//void visitNewStructures(NewStructures & pack) override;
	//void visitRazeStructures(RazeStructures & pack) override;
	//void visitSetAvailableCreatures(SetAvailableCreatures & pack) override;
	//void visitSetHeroesInTown(SetHeroesInTown & pack) override;
	//void visitHeroRecruited(HeroRecruited & pack) override;
	//void visitGiveHero(GiveHero & pack) override;
	//void visitSetObjectProperty(SetObjectProperty & pack) override;
	//void visitHeroLevelUp(HeroLevelUp & pack) override;
	//void visitCommanderLevelUp(CommanderLevelUp & pack) override;
	//void visitBattleStart(BattleStart & pack) override;
	//void visitBattleSetActiveStack(BattleSetActiveStack & pack) override;
	//void visitBattleTriggerEffect(BattleTriggerEffect & pack) override;
	//void visitBattleAttack(BattleAttack & pack) override;
	//void visitBattleSpellCast(BattleSpellCast & pack) override;
	//void visitSetStackEffect(SetStackEffect & pack) override;
	//void visitStacksInjured(StacksInjured & pack) override;
	//void visitBattleUnitsChanged(BattleUnitsChanged & pack) override;
	//void visitBattleObstaclesChanged(BattleObstaclesChanged & pack) override;
	//void visitBattleStackMoved(BattleStackMoved & pack) override;
	//void visitCatapultAttack(CatapultAttack & pack) override;
	//void visitPlayerStartsTurn(PlayerStartsTurn & pack) override;
	//void visitNewObject(NewObject & pack) override;
	//void visitSetAvailableArtifacts(SetAvailableArtifacts & pack) override;
	//void visitEntitiesChanged(EntitiesChanged & pack) override;
	//void visitSetCommanderProperty(SetCommanderProperty & pack) override;
	//void visitAddQuest(AddQuest & pack) override;
	//void visitChangeFormation(ChangeFormation & pack) override;
	//void visitChangeSpells(ChangeSpells & pack) override;
	//void visitSetAvailableHero(SetAvailableHero & pack) override;
	//void visitChangeObjectVisitors(ChangeObjectVisitors & pack) override;
	//void visitChangeArtifactsCostume(ChangeArtifactsCostume & pack) override;
	//void visitNewArtifact(NewArtifact & pack) override;
	//void visitBattleUpdateGateState(BattleUpdateGateState & pack) override;
	//void visitPlayerCheated(PlayerCheated & pack) override;
	//void visitDaysWithoutTown(DaysWithoutTown & pack) override;
	//void visitStartAction(StartAction & pack) override;
	//void visitSetRewardableConfiguration(SetRewardableConfiguration & pack) override;
	//void visitBattleSetStackProperty(BattleSetStackProperty & pack) override;
	//void visitBattleNextRound(BattleNextRound & pack) override;
	//void visitBattleCancelled(BattleCancelled & pack) override;
	//void visitBattleResultsApplied(BattleResultsApplied & pack) override;
	//void visitBattleResultAccepted(BattleResultAccepted & pack) override;
	//void visitTurnTimeUpdate(TurnTimeUpdate & pack) override;

public:
	AntilagRollbackGeneratorVisitor(const CGameState & gs)
		: gs(gs)
	{}

	bool canBeRolledBack() const;
	std::vector<std::unique_ptr<CPackForClient>> getRollbackPacks();
};

// Fake server that is used by client to make a quick prediction on what real server would reply without waiting for network latency
class AntilagServer final : public IGameServer, public INetworkConnectionListener, boost::noncopyable
{
	std::vector<std::shared_ptr<AntilagFakeConnection>> predictedReplies;
	std::shared_ptr<INetworkConnection> antilagNetConnection;
	std::shared_ptr<IGameConnection> antilagGameConnection;
	std::unique_ptr<CGameHandler> gameHandler;

	static constexpr uint32_t invalidPackageID = std::numeric_limits<uint32_t>::max();
	uint32_t currentPackageID = invalidPackageID;

	// IGameServer impl
	void setState(EServerState value) override;
	EServerState getState() const override;
	bool isPlayerHost(const PlayerColor & color) const override;
	bool hasPlayerAt(PlayerColor player, const std::shared_ptr<IGameConnection> & c) const override;
	bool hasBothPlayersAtSameConnection(PlayerColor left, PlayerColor right) const override;
	void broadcastPack(CPackForClient & pack) override;
	void onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage) override;
	void onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<std::byte> & message) override;

public:
	AntilagServer(INetworkHandler & network, const std::shared_ptr<CGameState> & gs);
	~AntilagServer();

	void tryPredictReply(const CPackForServer & request);
	bool verifyReply(const CPackForClient & reply);
};
