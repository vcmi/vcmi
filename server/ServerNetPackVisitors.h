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

#include "../lib/NetPackVisitor.h"

class ApplyGhNetPackVisitor : public VCMI_LIB_WRAP_NAMESPACE(ICPackVisitor)
{
private:
	bool result;
	CGameHandler & gh;
	CGameState & gs;

public:
	ApplyGhNetPackVisitor(CGameHandler & gh, CGameState & gs)
		:gh(gh), gs(gs), result(false)
	{
	}

	bool getResult() const
	{
		return result;
	}

	virtual void visitSaveGame(SaveGame & pack) override;
	virtual void visitEndTurn(EndTurn & pack) override;
	virtual void visitDismissHero(DismissHero & pack) override;
	virtual void visitMoveHero(MoveHero & pack) override;
	virtual void visitCastleTeleportHero(CastleTeleportHero & pack) override;
	virtual void visitArrangeStacks(ArrangeStacks & pack) override;
	virtual void visitBulkMoveArmy(BulkMoveArmy & pack) override;
	virtual void visitBulkSplitStack(BulkSplitStack & pack) override;
	virtual void visitBulkMergeStacks(BulkMergeStacks & pack) override;
	virtual void visitBulkSmartSplitStack(BulkSmartSplitStack & pack) override;
	virtual void visitDisbandCreature(DisbandCreature & pack) override;
	virtual void visitBuildStructure(BuildStructure & pack) override;
	virtual void visitRecruitCreatures(RecruitCreatures & pack) override;
	virtual void visitUpgradeCreature(UpgradeCreature & pack) override;
	virtual void visitGarrisonHeroSwap(GarrisonHeroSwap & pack) override;
	virtual void visitExchangeArtifacts(ExchangeArtifacts & pack) override;
	virtual void visitBulkExchangeArtifacts(BulkExchangeArtifacts & pack) override;
	virtual void visitAssembleArtifacts(AssembleArtifacts & pack) override;
	virtual void visitBuyArtifact(BuyArtifact & pack) override;
	virtual void visitTradeOnMarketplace(TradeOnMarketplace & pack) override;
	virtual void visitSetFormation(SetFormation & pack) override;
	virtual void visitHireHero(HireHero & pack) override;
	virtual void visitBuildBoat(BuildBoat & pack) override;
	virtual void visitQueryReply(QueryReply & pack) override;
	virtual void visitMakeAction(MakeAction & pack) override;
	virtual void visitMakeCustomAction(MakeCustomAction & pack) override;
	virtual void visitDigWithHero(DigWithHero & pack) override;
	virtual void visitCastAdvSpell(CastAdvSpell & pack) override;
	virtual void visitPlayerMessage(PlayerMessage & pack) override;
};