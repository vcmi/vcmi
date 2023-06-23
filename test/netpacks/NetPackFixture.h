/*
 * NetPackFixture.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/NetPacks.h"
#include "../../lib/gameState/CGameState.h"

namespace test
{

class GameStateFake : public CGameState
{
public:
	MOCK_METHOD3(updateEntity, void(Metatype, int32_t, const JsonNode &));
};

class NetPackFixture
{
public:
	NetPackFixture();
	virtual ~NetPackFixture();

	std::shared_ptr<GameStateFake> gameState;

protected:
	void setUp();

private:
};


}

