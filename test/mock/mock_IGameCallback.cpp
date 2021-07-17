/*
 * mock_IGameCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "mock_IGameCallback.h"

GameCallbackMock::GameCallbackMock(UpperCallback * upperCallback_)
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

void GameCallbackMock::sendAndApply(CPackForClient * pack)
{
	upperCallback->apply(pack);
}
