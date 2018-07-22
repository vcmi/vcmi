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

#include "mock_CPSICallback.h"

GameCallbackMock::GameCallbackMock()
{
	gs = nullptr;
	//battle = nullptr;
	//gs = (CGameState*)12356789;
}

GameCallbackMock::GameCallbackMock(CGameState * GS, boost::optional<PlayerColor> Player)
{
	gs = nullptr;
}

//std::vector<const CGTownInstance*> GameCallbackMock::getTownsInfo(bool onlyOur) const
//{
//	return std::vector<const CGTownInstance*>();
//}
