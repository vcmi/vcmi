/*
 * NetworkLagCompensator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "NetworkLagCompensator.h"

#include "NetworkLagPredictionTestVisitor.h"
#include "NetworkLagReplyPrediction.h"
#include "PackRollbackGeneratorVisitor.h"

#include "../CServerHandler.h"
#include "../Client.h"
#include "../GameEngine.h"
#include "../GameInstance.h"

#include "../../lib/serializer/BinarySerializer.h"
#include "../../lib/serializer/GameConnection.h"
#include "../../server/CGameHandler.h"

class RollbackNotSupportedException : public std::runtime_error
{
public:
	using std::runtime_error::runtime_error;
};

NetworkLagCompensator::NetworkLagCompensator(INetworkHandler & network, const std::shared_ptr<CGameState> & gs)
	: gameState(gs)
{
	antilagNetConnection = network.createAsyncConnection(*this);
	antilagGameConnection = std::make_shared<GameConnection>(antilagNetConnection);
}

NetworkLagCompensator::~NetworkLagCompensator() = default;

void NetworkLagCompensator::onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage)
{
	// should never be called
	throw std::runtime_error("AntilagServer::onDisconnected called!");
}

void NetworkLagCompensator::onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<std::byte> & message)
{
	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

	auto basePack = antilagGameConnection->retrievePack(message);
	auto * serverPack = dynamic_cast<CPackForServer *>(basePack.get());

	NetworkLagPredictionTestVisitor packVisitor;
	serverPack->visit(packVisitor);
	if(!packVisitor.canBeApplied())
		return;

	logGlobal->trace("Predicting effects of pack '%s'", typeid(*serverPack).name());

	NetworkLagReplyPrediction newPrediction;
	newPrediction.requestID = serverPack->requestID;
	newPrediction.senderID = serverPack->player;
	predictedReplies.push_back(std::move(newPrediction));

	try
	{
		CGameHandler gameHandler(*this, gameState);
		gameHandler.handleReceivedPack(GameConnectionID::FIRST_CONNECTION, *serverPack);
	}
	catch(const RollbackNotSupportedException &)
	{
		return;
	}
}

void NetworkLagCompensator::tryPredictReply(const CPackForServer & request)
{
	antilagGameConnection->sendPack(request);
	logGlobal->trace("Scheduled prediction of effects of pack '%s'", typeid(request).name());
}

bool NetworkLagCompensator::verifyReply(const CPackForClient & pack)
{
	logGlobal->trace("Verifying reply: received pack '%s'", typeid(pack).name());

	const auto * packageReceived = dynamic_cast<const PackageReceived *>(&pack);
	const auto * packageApplied = dynamic_cast<const PackageApplied *>(&pack);

	if(packageReceived)
	{
		assert(currentPackageID == invalidPackageID);

		if(!predictedReplies.empty() && predictedReplies.front().requestID == packageReceived->requestID)
		{
			[[maybe_unused]] const auto & nextPrediction = predictedReplies.front();
			assert(nextPrediction.senderID == packageReceived->player);
			assert(nextPrediction.requestID == packageReceived->requestID);
			currentPackageID = packageReceived->requestID;
		}
	}

	if(currentPackageID == invalidPackageID)
	{
		// this is system package or reply to actions of another player
		// TODO: consider reapplying all our predictions, in case if this event invalidated our prediction
		return false;
	}

	NetworkLagPredictedPack packWriter;
	BinarySerializer serializer(&packWriter);
	serializer & &pack;

	if(packWriter.predictedPackData == predictedReplies.front().writtenPacks.front().predictedPackData)
	{
		// Our prediction was sucessful - drop rollback information for this pack
		predictedReplies.front().writtenPacks.erase(predictedReplies.front().writtenPacks.begin());

		if(predictedReplies.front().writtenPacks.empty())
		{
			predictedReplies.erase(predictedReplies.begin());
			currentPackageID = invalidPackageID;
			return true;
		}
	}
	else
	{
		// Prediction was incorrect - rollback everything that is left in this prediction and use real server packs
		for(auto & prediction : boost::adaptors::reverse(predictedReplies.front().writtenPacks))
		{
			for(auto & pack : prediction.rollbackPacks)
				GAME->server().client->handlePack(*pack);
		}
		predictedReplies.erase(predictedReplies.begin());
		currentPackageID = invalidPackageID;
		return false;
	}

	if(packageApplied)
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

void NetworkLagCompensator::setState(EServerState value)
{
	// no-op
}

EServerState NetworkLagCompensator::getState() const
{
	return EServerState::GAMEPLAY;
}

bool NetworkLagCompensator::isPlayerHost(const PlayerColor & color) const
{
	return false; // TODO?
}

bool NetworkLagCompensator::hasPlayerAt(PlayerColor player, GameConnectionID c) const
{
	return true; // TODO?
}

bool NetworkLagCompensator::hasBothPlayersAtSameConnection(PlayerColor left, PlayerColor right) const
{
	return false; // TODO?
}

void NetworkLagCompensator::applyPack(CPackForClient & pack)
{
	PackRollbackGeneratorVisitor visitor(*gameState);
	pack.visit(visitor);
	if(!visitor.canBeRolledBack())
	{
		logGlobal->trace("Prediction not possible: pack '%s'", typeid(pack).name());
		throw RollbackNotSupportedException(std::string("Prediction not possible ") + typeid(pack).name());
	}

	logGlobal->trace("Prediction: pack '%s'", typeid(pack).name());

	NetworkLagPredictedPack packWriter;
	BinarySerializer serializer(&packWriter);
	serializer & &pack;
	packWriter.rollbackPacks = visitor.acquireRollbackPacks();
	predictedReplies.back().writtenPacks.push_back(std::move(packWriter));

	GAME->server().client->handlePack(pack);
}

void NetworkLagCompensator::sendPack(CPackForClient & pack, GameConnectionID connectionID)
{
	applyPack(pack);
}
