/*
 * ScriptedMovementClient.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "../../AI/Nullkiller2/AIGateway.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/callback/IClient.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/networkPacks/PacksForServer.h"
#include "../../lib/networkPacks/SaveLocalState.h"

/// Client that applies hero movement itself, so a test can script how the server answers
class ScriptedMovementClient : public IClient
{
public:
	explicit ScriptedMovementClient(std::shared_ptr<CGameState> gameState)
		: gameState(std::move(gameState))
	{
	}

	std::optional<BattleAction> makeSurrenderRetreatDecision(PlayerColor, const BattleID &, const BattleStateInfoForRetreat &) override
	{
		return std::nullopt;
	}

	int sendRequest(const CPackForServer & request, PlayerColor, bool) override
	{
		if(const auto * localState = dynamic_cast<const SaveLocalState *>(&request))
		{
			*gameState->getPlayerState(localState->player)->playerLocalSettings = localState->data;
			return ++requestID;
		}

		const auto * movement = dynamic_cast<const MoveHero *>(&request);
		auto * hero = movement ? gameState->getHero(movement->hid) : nullptr;
		if(!hero || movement->path.empty())
			return ++requestID;

		++movementRequests;
		const int3 destination = movement->path.front();
		requestedTiles.push_back(hero->convertToVisitablePos(destination));

		TryMoveHero reply;
		reply.id = hero->id;
		reply.start = hero->pos;
		reply.end = destination;
		reply.result = failing ? TryMoveHero::FAILED : TryMoveHero::SUCCESS;
		reply.movePoints = movementLeft.value_or(hero->movementPointsRemaining());

		gameState->apply(reply);
		if(afterMovement)
			afterMovement();
		if(gateway)
			gateway->heroMoved(reply, false);

		return ++requestID;
	}

	/// Movement results are reported to this gateway, like the server does
	void notify(NK2AI::AIGateway & receiver)
	{
		gateway = &receiver;
	}

	/// Every movement leaves the hero with given movement points, e.g. as if a map event drained them
	void drainMovement(int pointsLeft)
	{
		movementLeft = pointsLeft;
	}

	/// Every movement is rejected
	void failMovement()
	{
		failing = true;
	}

	bool requested(const int3 & tile) const
	{
		return vstd::contains(requestedTiles, tile);
	}

	int movementRequests = 0;

	/// Runs after every applied movement, e.g. to emulate a map event visited on the way
	std::function<void()> afterMovement;

private:
	std::shared_ptr<CGameState> gameState;
	NK2AI::AIGateway * gateway = nullptr;
	std::vector<int3> requestedTiles;
	std::optional<int> movementLeft;
	bool failing = false;
	int requestID = 0;
};
