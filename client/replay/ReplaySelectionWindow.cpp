/*
 * ReplaySelectionWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ReplaySelectionWindow.h"

#include "GameplayReplayer.h"

#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../Client.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/WindowHandler.h"
#include "../windows/GUIClasses.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/gameState/ReplayLog.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/MetaString.h"

namespace
{
	GameplayReplayer::Options currentOptions()
	{
		GameplayReplayer::Options options;
		options.showBattles = settings["adventure"]["replayShowBattles"].Bool();
		return options;
	}

	std::string describeTurn(const ReplayTurnOption & turn)
	{
		const auto * playerState = GAME->interface()->cb->getPlayerState(turn.player, false);
		const std::string playerName = playerState ? playerState->getNameTranslated() : turn.player.toString();

		MetaString text = MetaString::createFromTextID(turn.ongoing ? "vcmi.replay.turnOngoing" : "vcmi.replay.turn");
		text.replaceNumber(turn.day);
		text.replaceRawString(playerName);
		return text.toString();
	}

	void startReplay(ReplaySequence sequence)
	{
		const PlayerColor observer = GAME->interface()->playerID;
		GAME->server().replayer().start(std::move(sequence), observer, currentOptions());
	}
}

void ReplaySelection::showSelectionDialog()
{
	// with simultaneous turns another player handled by this client could act while the live
	// session is swapped out, and its actions would be lost
	if(GAME->server().client->gameState().actingPlayers.size() > 1)
	{
		GAME->interface()->showInfoDialog(LIBRARY->generaltexth->translate("vcmi.replay.notNow"));
		return;
	}

	const ReplayLog & log = GAME->server().client->gameState().replayLog;
	const auto turns = ReplayPlanner::availableTurns(log);
	const bool entireGame = log.canReplayEntireGame();

	if(turns.empty() && !entireGame)
	{
		GAME->interface()->showInfoDialog(LIBRARY->generaltexth->translate("vcmi.replay.nothingRecorded"));
		return;
	}

	std::vector<std::string> entries;

	// newest turn first - that is the one the player is most likely after
	std::vector<ReplayTurnOption> orderedTurns(turns.rbegin(), turns.rend());
	for(const auto & turn : orderedTurns)
		entries.push_back(describeTurn(turn));

	if(entireGame)
		entries.push_back(LIBRARY->generaltexth->translate("vcmi.replay.entireGame"));

	auto onSelected = [orderedTurns, entireGame](int index)
	{
		try
		{
			const ReplayLog & currentLog = GAME->server().client->gameState().replayLog;

			if(index < static_cast<int>(orderedTurns.size()))
				startReplay(ReplayPlanner::prepareTurn(currentLog, orderedTurns[index]));
			else if(entireGame)
				startReplay(ReplayPlanner::prepareEntireGame(currentLog));
		}
		catch(const std::exception & e)
		{
			logGlobal->error("Failed to prepare a replay: %s", e.what());
			GAME->interface()->showInfoDialog(LIBRARY->generaltexth->translate("vcmi.replay.failed"));
		}
	};

	ENGINE->windows().createAndPushWindow<CObjectListWindow>(
		entries,
		nullptr,
		LIBRARY->generaltexth->translate("vcmi.replay.replayTurn"),
		LIBRARY->generaltexth->translate("vcmi.replay.replayTurn.help"),
		onSelected);
}
