/*
 * AntilagServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AntilagServer.h"

#include "CServerHandler.h"
#include "Client.h"
#include "GameEngine.h"

#include "../server/CGameHandler.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/serializer/GameConnection.h"
#include "GameInstance.h"

class AntilagRollbackNotSupportedException : public std::runtime_error
{
public:
	using std::runtime_error::runtime_error;
};


int ConnectionPackWriter::write(const std::byte * data, unsigned size)
{
	buffer.insert(buffer.end(), data, data + size);
	return size;
}

void AntilagRollbackGeneratorVisitor::visitPackageReceived(PackageReceived & pack)
{
	success = true;
	// no-op rollback?
}

void AntilagRollbackGeneratorVisitor::visitPackageApplied(PackageApplied & pack)
{
	success = true;
	// no-op rollback?
}

void AntilagRollbackGeneratorVisitor::visitPlayerBlocked(PlayerBlocked & pack)
{
	success = true;
	// no-op rollback?
}

void AntilagRollbackGeneratorVisitor::visitSwapStacks(SwapStacks & pack)
{
	auto rollbackSwap = std::make_unique<SwapStacks>();

	rollbackSwap->srcArmy = pack.dstArmy;
	rollbackSwap->dstArmy = pack.srcArmy;
	rollbackSwap->srcSlot = pack.dstSlot;
	rollbackSwap->dstSlot = pack.srcSlot;

	rollbackPacks.push_back(std::move(rollbackSwap));
	success = true;
}

void AntilagRollbackGeneratorVisitor::visitRebalanceStacks(RebalanceStacks & pack)
{
	const auto * srcObject = gs.getObjInstance(pack.srcArmy);
	const auto * dstObject = gs.getObjInstance(pack.dstArmy);

	const auto * srcArmy = dynamic_cast<const CArmedInstance*>(srcObject);
	const auto * dstArmy = dynamic_cast<const CArmedInstance*>(dstObject);

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

void AntilagRollbackGeneratorVisitor::visitBulkRebalanceStacks(BulkRebalanceStacks & pack)
{
	for (auto & subpack : pack.moves)
		visitRebalanceStacks(subpack);

	success = true;
}

void AntilagRollbackGeneratorVisitor::visitHeroVisitCastle(HeroVisitCastle & pack)
{
	auto rollbackVisit = std::make_unique<HeroVisitCastle>();
	rollbackVisit->startVisit = !pack.startVisit;
	rollbackVisit->tid = pack.tid;
	rollbackVisit->hid = pack.hid;

	rollbackPacks.push_back(std::move(rollbackVisit));

	success = true;
}

void AntilagRollbackGeneratorVisitor::visitTryMoveHero(TryMoveHero & pack)
{
	auto rollbackMove = std::make_unique<TryMoveHero>();
	auto rollbackFow = std::make_unique<FoWChange>();
	const auto * movedHero = gs.getHero(pack.id);

	rollbackMove->id = pack.id;
	rollbackMove->movePoints = movedHero->movementPointsRemaining();
	rollbackMove->result = pack.result;
	if (pack.result == TryMoveHero::EMBARK)
		rollbackMove->result = TryMoveHero::DISEMBARK;
	if (pack.result == TryMoveHero::DISEMBARK)
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

bool AntilagRollbackGeneratorVisitor::canBeRolledBack() const
{
	return success;
}

std::vector<std::unique_ptr<CPackForClient>> AntilagRollbackGeneratorVisitor::getRollbackPacks()
{
	return std::move(rollbackPacks);
}

AntilagReplyPredictionVisitor::AntilagReplyPredictionVisitor() = default;

void AntilagReplyPredictionVisitor::visitMoveHero(MoveHero & pack)
{
	canBeAppliedValue = true;
}

void AntilagReplyPredictionVisitor::visitArrangeStacks(ArrangeStacks & pack)
{
	canBeAppliedValue = true;
}

bool AntilagReplyPredictionVisitor::canBeApplied() const
{
	return canBeAppliedValue;
}

AntilagServer::AntilagServer(INetworkHandler & network, const std::shared_ptr<CGameState> & gs)
	: gameState(gs)
{
	antilagNetConnection = network.createAsyncConnection(*this);
	antilagGameConnection = std::make_shared<GameConnection>(antilagNetConnection);
}

AntilagServer::~AntilagServer() = default;

void AntilagServer::onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage)
{
	// should never be called
	throw std::runtime_error("AntilagServer::onDisconnected called!");
}

void AntilagServer::onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<std::byte> & message)
{
	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

	auto basePack = antilagGameConnection->retrievePack(message);
	auto * serverPack = dynamic_cast<CPackForServer*>(basePack.get());

	AntilagReplyPredictionVisitor packVisitor;
	serverPack->visit(packVisitor);
	if (!packVisitor.canBeApplied())
		return;

	logGlobal->info("Predicting effects of pack '%s'", typeid(*serverPack).name());

	AntilagReplyPrediction newPrediction;
	newPrediction.requestID = serverPack->requestID;
	newPrediction.senderID = serverPack->player;
	predictedReplies.push_back(std::move(newPrediction));

	try
	{
		CGameHandler gameHandler(*this, gameState);
		gameHandler.handleReceivedPack(GameConnectionID::FIRST_CONNECTION, *serverPack);
	}
	catch (const AntilagRollbackNotSupportedException & )
	{
		return;
	}
}

void AntilagServer::tryPredictReply(const CPackForServer & request)
{
	antilagGameConnection->sendPack(request);
	logGlobal->info("Scheduled prediction of effects of pack '%s'", typeid(request).name());
}

bool AntilagServer::verifyReply(const CPackForClient & pack)
{
	logGlobal->info("Verifying reply: received pack '%s'", typeid(pack).name());

	const auto * packageReceived = dynamic_cast<const PackageReceived*>(&pack);
	const auto * packageApplied = dynamic_cast<const PackageApplied*>(&pack);

	if (packageReceived)
	{
		assert(currentPackageID == invalidPackageID);

		if (!predictedReplies.empty() && predictedReplies.front().requestID == packageReceived->requestID)
		{
			[[maybe_unused]] const auto & nextPrediction = predictedReplies.front();
			assert(nextPrediction.senderID == packageReceived->player);
			assert(nextPrediction.requestID == packageReceived->requestID);
			currentPackageID = packageReceived->requestID;
		}
	}

	if (currentPackageID == invalidPackageID)
	{
		// this is system package or reply to actions of another player
		// TODO: consider reapplying all our predictions, in case if this event invalidated our prediction
		return false;
	}

	ConnectionPackWriter packWriter;
	BinarySerializer serializer(&packWriter);
	serializer & &pack;

	if (packWriter.buffer == predictedReplies.front().writtenPacks.front().buffer)
	{
		// Our prediction was sucessful - drop rollback information for this pack
		predictedReplies.front().writtenPacks.erase(predictedReplies.front().writtenPacks.begin());

		if (predictedReplies.front().writtenPacks.empty())
		{
			predictedReplies.erase(predictedReplies.begin());
			currentPackageID = invalidPackageID;
			return true;
		}
	}
	else
	{
		// Prediction was incorrect - rollback everything that is left in this prediction and use real server packs
		for (auto & prediction : boost::adaptors::reverse(predictedReplies.front().writtenPacks))
		{
			for (auto & pack : prediction.rollbackPacks)
				GAME->server().client->handlePack(*pack);
		}
		predictedReplies.erase(predictedReplies.begin());
		currentPackageID = invalidPackageID;
		return false;
	}

	if (packageApplied)
	{
		assert(currentPackageID == packageApplied->requestID);
		assert(!predictedReplies.empty());
		assert(currentPackageID == predictedReplies.front().requestID);
		assert(predictedReplies.front().writtenPacks.empty());
		predictedReplies.erase(predictedReplies.begin());
		currentPackageID = invalidPackageID;
	}

	return true;
}

void AntilagServer::setState(EServerState value)
{
	// no-op
}

EServerState AntilagServer::getState() const
{
	return EServerState::GAMEPLAY;
}

bool AntilagServer::isPlayerHost(const PlayerColor & color) const
{
	return false; // TODO?
}

bool AntilagServer::hasPlayerAt(PlayerColor player, GameConnectionID c) const
{
	return true; // TODO?
}

bool AntilagServer::hasBothPlayersAtSameConnection(PlayerColor left, PlayerColor right) const
{
	return false; // TODO?
}

void AntilagServer::applyPack(CPackForClient & pack)
{
	AntilagRollbackGeneratorVisitor visitor(*gameState);
	pack.visit(visitor);
	if (!visitor.canBeRolledBack())
	{
		logGlobal->info("Prediction not possible: pack '%s'", typeid(pack).name());
		throw AntilagRollbackNotSupportedException(std::string("Prediction not possible ") + typeid(pack).name());
	}

	logGlobal->info("Prediction: pack '%s'", typeid(pack).name());

	ConnectionPackWriter packWriter;
	BinarySerializer serializer(&packWriter);
	serializer & &pack;
	packWriter.rollbackPacks = visitor.getRollbackPacks();
	predictedReplies.back().writtenPacks.push_back(std::move(packWriter));

	GAME->server().client->handlePack(pack);
}

void AntilagServer::sendPack(CPackForClient & pack, GameConnectionID connectionID)
{
	applyPack(pack);
}
