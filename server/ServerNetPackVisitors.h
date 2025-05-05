/*
 * ServerNetPackVisitors.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/networkPacks/NetPackVisitor.h"

class ApplyGhNetPackVisitor : public VCMI_LIB_WRAP_NAMESPACE(ICPackVisitor)
{
private:
	std::shared_ptr<CConnection> connection;
	CGameHandler & gh;
	bool result;

public:
	ApplyGhNetPackVisitor(CGameHandler & gh, const std::shared_ptr<CConnection> & connection)
		: connection(connection)
		, gh(gh)
		, result(false)
	{
	}

	bool getResult() const
	{
		return result;
	}

	void visitSaveGame(SaveGame & pack) override;
	void visitGamePause(GamePause & pack) override;
	void visitEndTurn(EndTurn & pack) override;
	void visitDismissHero(DismissHero & pack) override;
	void visitMoveHero(MoveHero & pack) override;
	void visitCastleTeleportHero(CastleTeleportHero & pack) override;
	void visitArrangeStacks(ArrangeStacks & pack) override;
	void visitBulkMoveArmy(BulkMoveArmy & pack) override;
	void visitBulkSplitStack(BulkSplitStack & pack) override;
	void visitBulkMergeStacks(BulkMergeStacks & pack) override;
	void visitBulkSmartSplitStack(BulkSplitAndRebalanceStack & pack) override;
	void visitDisbandCreature(DisbandCreature & pack) override;
	void visitBuildStructure(BuildStructure & pack) override;
	void visitSpellResearch(SpellResearch & pack) override;
	void visitVisitTownBuilding(VisitTownBuilding & pack) override;
	void visitRecruitCreatures(RecruitCreatures & pack) override;
	void visitUpgradeCreature(UpgradeCreature & pack) override;
	void visitGarrisonHeroSwap(GarrisonHeroSwap & pack) override;
	void visitExchangeArtifacts(ExchangeArtifacts & pack) override;
	void visitBulkExchangeArtifacts(BulkExchangeArtifacts & pack) override;
	void visitManageBackpackArtifacts(ManageBackpackArtifacts & pack) override;
	void visitManageEquippedArtifacts(ManageEquippedArtifacts & pack) override;
	void visitAssembleArtifacts(AssembleArtifacts & pack) override;
	void visitEraseArtifactByClient(EraseArtifactByClient & pack) override;
	void visitBuyArtifact(BuyArtifact & pack) override;
	void visitTradeOnMarketplace(TradeOnMarketplace & pack) override;
	void visitSetFormation(SetFormation & pack) override;
	void visitHireHero(HireHero & pack) override;
	void visitBuildBoat(BuildBoat & pack) override;
	void visitQueryReply(QueryReply & pack) override;
	void visitMakeAction(MakeAction & pack) override;
	void visitDigWithHero(DigWithHero & pack) override;
	void visitCastAdvSpell(CastAdvSpell & pack) override;
	void visitPlayerMessage(PlayerMessage & pack) override;
	void visitSaveLocalState(SaveLocalState & pack) override;
};
