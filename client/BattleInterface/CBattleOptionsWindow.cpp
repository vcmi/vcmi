#include "StdInc.h"
#include "CBattleOptionsWindow.h"

#include "CBattleInterface.h"
#include "../GUIClasses.h"
#include "../AdventureMapButton.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../UIFramework/CGuiHandler.h"

CBattleOptionsWindow::CBattleOptionsWindow(const SDL_Rect & position, CBattleInterface *owner): myInt(owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos = position;
	background = new CPicture("comopbck.bmp");
	background->colorize(owner->curInt->playerID);

	viewGrid = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintCellBorders, owner, true), boost::bind(&CBattleInterface::setPrintCellBorders, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[427].first)(3,CGI->generaltexth->zelp[427].first), CGI->generaltexth->zelp[427].second, false, "sysopchk.def", NULL, 25, 56, false);
	viewGrid->select(owner->curInt->sysOpts.printCellBorders);
	movementShadow = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintStackRange, owner, true), boost::bind(&CBattleInterface::setPrintStackRange, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[428].first)(3,CGI->generaltexth->zelp[428].first), CGI->generaltexth->zelp[428].second, false, "sysopchk.def", NULL, 25, 89, false);
	movementShadow->select(owner->curInt->sysOpts.printStackRange);
	mouseShadow = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintMouseShadow, owner, true), boost::bind(&CBattleInterface::setPrintMouseShadow, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[429].first)(3,CGI->generaltexth->zelp[429].first), CGI->generaltexth->zelp[429].second, false, "sysopchk.def", NULL, 25, 122, false);
	mouseShadow->select(owner->curInt->sysOpts.printMouseShadow);

	animSpeeds = new CHighlightableButtonsGroup(0);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[422].first),CGI->generaltexth->zelp[422].second, "sysopb9.def", 28, 225, 1);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[423].first),CGI->generaltexth->zelp[423].second, "sysob10.def", 92, 225, 2);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[424].first),CGI->generaltexth->zelp[424].second, "sysob11.def",156, 225, 4);
	animSpeeds->select(owner->getAnimSpeed(), 1);
	animSpeeds->onChange = boost::bind(&CBattleInterface::setAnimSpeed, owner, _1);

	setToDefault = new AdventureMapButton (CGI->generaltexth->zelp[393], boost::bind(&CBattleOptionsWindow::bDefaultf,this), 246, 359, "codefaul.def");
	setToDefault->swappedImages = true;
	setToDefault->update();
	exit = new AdventureMapButton (CGI->generaltexth->zelp[392], boost::bind(&CBattleOptionsWindow::bExitf,this), 357, 359, "soretrn.def",SDLK_RETURN);
	exit->swappedImages = true;
	exit->update();

	//creating labels
	labels.push_back(new CLabel(242,  32, FONT_BIG,    CENTER, tytulowy, CGI->generaltexth->allTexts[392]));//window title
	labels.push_back(new CLabel(122, 214, FONT_MEDIUM, CENTER, tytulowy, CGI->generaltexth->allTexts[393]));//animation speed
	labels.push_back(new CLabel(122, 293, FONT_MEDIUM, CENTER, tytulowy, CGI->generaltexth->allTexts[394]));//music volume
	labels.push_back(new CLabel(122, 359, FONT_MEDIUM, CENTER, tytulowy, CGI->generaltexth->allTexts[395]));//effects' volume
	labels.push_back(new CLabel(353,  66, FONT_MEDIUM, CENTER, tytulowy, CGI->generaltexth->allTexts[396]));//auto - combat options
	labels.push_back(new CLabel(353, 265, FONT_MEDIUM, CENTER, tytulowy, CGI->generaltexth->allTexts[397]));//creature info

	//auto - combat options
	labels.push_back(new CLabel(283,  86, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[398]));//creatures
	labels.push_back(new CLabel(283, 116, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[399]));//spells
	labels.push_back(new CLabel(283, 146, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[400]));//catapult
	labels.push_back(new CLabel(283, 176, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[151]));//ballista
	labels.push_back(new CLabel(283, 206, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[401]));//first aid tent

	//creature info
	labels.push_back(new CLabel(283, 285, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[402]));//all stats
	labels.push_back(new CLabel(283, 315, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[403]));//spells only

	//general options
	labels.push_back(new CLabel(61,  57, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[404]));
	labels.push_back(new CLabel(61,  90, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[405]));
	labels.push_back(new CLabel(61, 123, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[406]));
	labels.push_back(new CLabel(61, 156, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[407]));
}

void CBattleOptionsWindow::bDefaultf()
{
}

void CBattleOptionsWindow::bExitf()
{
	GH.popIntTotally(this);
}