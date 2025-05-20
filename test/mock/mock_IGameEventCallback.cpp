/*
 * mock_IGameEventCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "mock_IGameEventCallback.h"

GameEventCallbackMock::GameEventCallbackMock(UpperCallback * upperCallback_)
	: upperCallback(upperCallback_)
{

}

GameEventCallbackMock::~GameEventCallbackMock() = default;

void GameEventCallbackMock::sendAndApply(CPackForClient & pack)
{
	upperCallback->apply(pack);
}

vstd::RNG & GameEventCallbackMock::getRandomGenerator()
{
	throw std::runtime_error("Not implemented!");
}
