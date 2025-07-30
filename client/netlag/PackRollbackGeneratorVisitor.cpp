/*
 * PackRollbackGeneratorVisitor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "PackRollbackGeneratorVisitor.h"

#include "../lib/gameState/CGameState.h"
#include "../lib/mapObjects/CGHeroInstance.h"

void PackRollbackGeneratorVisitor::visitPackageReceived(PackageReceived & pack)
{
	success = true;
	// no-op rollback?
}

void PackRollbackGeneratorVisitor::visitPackageApplied(PackageApplied & pack)
{
	success = true;
	// no-op rollback?
}

void PackRollbackGeneratorVisitor::visitPlayerBlocked(PlayerBlocked & pack)
{
	success = true;
	// no-op rollback?
}

void PackRollbackGeneratorVisitor::visitSwapStacks(SwapStacks & pack)
{
	auto rollbackSwap = std::make_unique<SwapStacks>();

	rollbackSwap->srcArmy = pack.dstArmy;
	rollbackSwap->dstArmy = pack.srcArmy;
	rollbackSwap->srcSlot = pack.dstSlot;
	rollbackSwap->dstSlot = pack.srcSlot;

	rollbackPacks.push_back(std::move(rollbackSwap));
	success = true;
}

void PackRollbackGeneratorVisitor::visitRebalanceStacks(RebalanceStacks & pack)
{
	const auto * srcObject = gs.getObjInstance(pack.srcArmy);
	const auto * dstObject = gs.getObjInstance(pack.dstArmy);

	const auto * srcArmy = dynamic_cast<const CArmedInstance *>(srcObject);
	const auto * dstArmy = dynamic_cast<const CArmedInstance *>(dstObject);

	if (srcArmy->getStack(pack.srcSlot).getTotalExperience() != 0 ||
	   dstArmy->getStack(pack.srcSlot).getTotalExperience() != 0 ||
	   srcArmy->getStack(pack.srcSlot).getSlot(ArtifactPosition::CREATURE_SLOT)->artifactID.hasValue())
	{
		// TODO: rollback creature artifacts & stack experience
		return;
	}

	auto rollbackRebalance = std::make_unique<RebalanceStacks>();
	rollbackRebalance->srcArmy = pack.dstArmy;
	rollbackRebalance->dstArmy = pack.srcArmy;
	rollbackRebalance->srcSlot = pack.dstSlot;
	rollbackRebalance->dstSlot = pack.srcSlot;
	rollbackRebalance->count = pack.count;
	rollbackPacks.push_back(std::move(rollbackRebalance));
	success = true;
}

void PackRollbackGeneratorVisitor::visitBulkRebalanceStacks(BulkRebalanceStacks & pack)
{
	for(auto & subpack : pack.moves)
		visitRebalanceStacks(subpack);

	success = true;
}

void PackRollbackGeneratorVisitor::visitHeroVisitCastle(HeroVisitCastle & pack)
{
	auto rollbackVisit = std::make_unique<HeroVisitCastle>();
	rollbackVisit->startVisit = !pack.startVisit;
	rollbackVisit->tid = pack.tid;
	rollbackVisit->hid = pack.hid;

	rollbackPacks.push_back(std::move(rollbackVisit));

	success = true;
}

void PackRollbackGeneratorVisitor::visitTryMoveHero(TryMoveHero & pack)
{
	auto rollbackMove = std::make_unique<TryMoveHero>();
	auto rollbackFow = std::make_unique<FoWChange>();
	const auto * movedHero = gs.getHero(pack.id);

	rollbackMove->id = pack.id;
	rollbackMove->movePoints = movedHero->movementPointsRemaining();
	rollbackMove->result = pack.result;
	if(pack.result == TryMoveHero::EMBARK)
		rollbackMove->result = TryMoveHero::DISEMBARK;
	if(pack.result == TryMoveHero::DISEMBARK)
		rollbackMove->result = TryMoveHero::EMBARK;
	rollbackMove->start = pack.end;
	rollbackMove->end = pack.start;

	rollbackFow->mode = ETileVisibility::HIDDEN;
	rollbackFow->player = movedHero->getOwner();
	rollbackFow->tiles = pack.fowRevealed;

	rollbackPacks.push_back(std::move(rollbackMove));
	rollbackPacks.push_back(std::move(rollbackFow));
	success = true;
}

bool PackRollbackGeneratorVisitor::canBeRolledBack() const
{
	return success;
}

std::vector<std::unique_ptr<CPackForClient>> PackRollbackGeneratorVisitor::acquireRollbackPacks()
{
	return std::move(rollbackPacks);
}
