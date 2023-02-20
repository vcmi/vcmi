/*
 * CPuzzleWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CPuzzleWindow.h"

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../adventureMap/CResDataBar.h"
#include "../gui/CGuiHandler.h"
#include "../gui/TextAlignment.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"

#include "../../CCallback.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/StartInfo.h"

CPuzzleWindow::CPuzzleWindow(const int3 & GrailPos, double discoveredRatio)
	: CWindowObject(PLAYER_COLORED | BORDERED, "PUZZLE"),
	grailPos(GrailPos),
	currentAlpha(SDL_ALPHA_OPAQUE)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	CCS->soundh->playSound(soundBase::OBELISK);

	quitb = std::make_shared<CButton>(Point(670, 538), "IOK6432.DEF", CButton::tooltip(CGI->generaltexth->allTexts[599]), std::bind(&CPuzzleWindow::close, this), SDLK_RETURN);
	quitb->assignedKeys.insert(SDLK_ESCAPE);
	quitb->setBorderColor(Colors::METALLIC_GOLD);

	logo = std::make_shared<CPicture>("PUZZLOGO", 607, 3);
	title = std::make_shared<CLabel>(700, 95, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[463]);
	resDataBar = std::make_shared<CResDataBar>("ARESBAR.bmp", 3, 575, 32, 2, 85, 85);

	int faction = LOCPLINT->cb->getStartInfo()->playerInfos.find(LOCPLINT->playerID)->second.castle;

	auto & puzzleMap = (*CGI->townh)[faction]->puzzleMap;

	for(auto & elem : puzzleMap)
	{
		const SPuzzleInfo & info = elem;

		auto piece = std::make_shared<CPicture>(info.filename, info.x, info.y);

		//piece that will slowly disappear
		if(info.whenUncovered <= GameConstants::PUZZLE_MAP_PIECES * discoveredRatio)
		{
			piecesToRemove.push_back(piece);
			piece->needRefresh = true;
			piece->recActions = piece->recActions & ~SHOWALL;
		}
		else
		{
			visiblePieces.push_back(piece);
		}
	}
}

void CPuzzleWindow::showAll(SDL_Surface * to)
{
	assert(0);
	//int3 moveInt = int3(8, 9, 0);
	//Rect mapRect = Rect(Point(pos.x + 8, pos.y + 7), Point(544, 591));
	//int3 topTile = grailPos - moveInt;

	//MapDrawingInfo info(topTile, LOCPLINT->cb->getVisibilityMap(), mapRect);
	//info.puzzleMode = true;
	//info.grailPos = grailPos;
	//CGI->mh->drawTerrainRectNew(to, &info);

	CWindowObject::showAll(to);
}

void CPuzzleWindow::show(SDL_Surface * to)
{
	static int animSpeed = 2;

	if(currentAlpha < animSpeed)
	{
		piecesToRemove.clear();
	}
	else
	{
		//update disappearing puzzles
		for(auto & piece : piecesToRemove)
			piece->setAlpha(currentAlpha);
		currentAlpha -= animSpeed;
	}
	CWindowObject::show(to);
}
