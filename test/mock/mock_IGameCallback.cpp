/*
 * {file}.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "mock_IGameCallback.h"

GameCallbackMock::GameCallbackMock(const UpperCallback * upperCallback_)
	: upperCallback(upperCallback_)
{

}

GameCallbackMock::~GameCallbackMock()
{

}

void GameCallbackMock::setGameState(CGameState * gameState)
{
	gs = gameState;
}

void GameCallbackMock::commitPackage(CPackForClient * pack)
{
	sendAndApply(pack);
}

void GameCallbackMock::sendAndApply(CPackForClient * info)
{
	upperCallback->sendAndApply(info);
}
