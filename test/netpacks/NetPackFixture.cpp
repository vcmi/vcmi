/*
 * NetPackFixture.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "NetPackFixture.h"
namespace test
{


NetPackFixture::NetPackFixture()
{

}

NetPackFixture::~NetPackFixture() = default;

void NetPackFixture::setUp()
{
    gameState = std::make_shared<GameStateFake>();
}

}
