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

#include "NetworkLagIndependentPackVisitor.h"
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

bool NetworkLagCompensator::verifyReply(CPackForClient & pack)
{
	logGlobal->trace("Verifying reply: received pack '%s'", typeid(pack).name());

	const auto * packageReceived = dynamic_cast<const PackageReceived *>(&pack);

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
		// unconfirmed predictions were applied on top of state that this pack may change, so they can not be kept
		NetworkLagIndependentPackVisitor visitor;
		pack.visit(visitor);
		if(!visitor.isIndependent())
			rollbackAllPredictions();
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
			// predicted PackageApplied was not delivered to client, so the one from server must be
			return dynamic_cast<const PackageApplied *>(&pack) == nullptr;
		}
	}
	else
	{
		// Prediction was incorrect. Later predictions were applied on top of this one and may depend on it,
		// so all of them are rolled back and real server packs are used instead
		rollbackAllPredictions();
		return false;
	}

	return true;
}

void NetworkLagCompensator::rollbackAllPredictions()
{
	for(auto & reply : std::views::reverse(predictedReplies))
	{
		for(auto & prediction : std::views::reverse(reply.writtenPacks))
		{
			for(auto & pack : prediction.rollbackPacks)
				GAME->server().client->handlePack(*pack);
		}
	}
	predictedReplies.clear();
	currentPackageID = invalidPackageID;
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

	// Client reacts to PackageApplied by sending follow-up requests, such as next step of hero movement.
	// Such requests can not be rolled back, so the client only receives PackageApplied once server confirms it
	if(dynamic_cast<const PackageApplied *>(&pack) != nullptr)
		return;

	GAME->server().client->handlePack(pack);
}

void NetworkLagCompensator::sendPack(CPackForClient & pack, GameConnectionID connectionID)
{
	applyPack(pack);
}
