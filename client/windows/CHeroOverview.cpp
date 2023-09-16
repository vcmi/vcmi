/*
 * CHeroOverview.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CHeroOverview.h"

#include "../CGameInfo.h"
#include "../gui/CGuiHandler.h"
#include "../render/Colors.h"
#include "../render/Graphics.h"
#include "../widgets/CComponent.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"

#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"

CHeroOverview::CHeroOverview(const HeroTypeID & h)
	: CWindowObject(BORDERED | RCLICK_POPUP), hero { h }, heroIndex { h.getNum() }
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

    genHeroWindow();

	center();
}

void CHeroOverview::genHeader()
{
	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), pos);
	updateShadow();

	labelTitle = std::make_shared<CLabel>(pos.w / 2 + 8, 21, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[77]);
	labelSubTitle = std::make_shared<CLabel>(pos.w / 2, 88, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, (*CGI->heroh)[heroIndex]->getNameTranslated() + " - " + (*CGI->heroh)[heroIndex]->heroClass->getNameTranslated());
	image = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsSmall"), (*CGI->heroh)[heroIndex]->imageIndex, 0, pos.w / 2 - 24, 45);
}

void CHeroOverview::genHeroWindow()
{
	pos = Rect(0, 0, 292, 226);
	genHeader();
	labelHeroSpeciality = std::make_shared<CLabel>(pos.w / 2 + 4, 117, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[78]);
	auto heroIndex = hero.getNum() >= CGI->heroh->size() ? 0 : hero.getNum();

	imageSpeciality = std::make_shared<CAnimImage>(AnimationPath::builtin("UN44"), (*CGI->heroh)[heroIndex]->imageIndex, 0, pos.w / 2 - 22, 134);
	labelSpecialityName = std::make_shared<CLabel>(pos.w / 2, 188, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, (*CGI->heroh)[heroIndex]->getSpecialtyNameTranslated());
}