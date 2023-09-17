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
#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../render/Graphics.h"
#include "../render/IImage.h"
#include "../renderSDL/RenderHandler.h"
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

    int yOffset = 35;
    int borderOffset = 5;

    ColorRGBA borderColor = Colors::WHITE;

    Canvas canvas = Canvas(Point(pos.w, pos.h));

    // Image
    canvas.drawBorder(Rect(borderOffset - 1, borderOffset + yOffset - 1, 58 + 2, 64 + 2), borderColor);

    // Namebox
    canvas.drawColorBlended(Rect(64 + borderOffset, borderOffset + yOffset, 220, 64), ColorRGBA(0, 0, 0, 75));
    canvas.drawBorder(Rect(64 + borderOffset - 1, borderOffset + yOffset - 1, 220 + 2, 64 + 2), borderColor);

    // vertical line
    canvas.drawLine(Point(295, borderOffset + yOffset - 1), Point(295, 445), borderColor, borderColor);
    
    std::shared_ptr<IImage> backgroundShapesImg = GH.renderHandler().createImage(canvas.getInternalSurface());
    backgroundShapes = std::make_shared<CPicture>(backgroundShapesImg, pos);

	labelTitle = std::make_shared<CLabel>(pos.w / 2 + 8, 21, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[77]);

    // Image
	image = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), (*CGI->heroh)[heroIndex]->imageIndex, 0, borderOffset, borderOffset + yOffset);

    // Namebox
	labelHeroName = std::make_shared<CLabel>(64 + borderOffset + 110, borderOffset + yOffset + 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, (*CGI->heroh)[heroIndex]->getNameTranslated());
	labelHeroClass = std::make_shared<CLabel>(64 + borderOffset + 110, borderOffset + yOffset + 45, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, (*CGI->heroh)[heroIndex]->heroClass->getNameTranslated());
}

void CHeroOverview::genHeroWindow()
{
	pos = Rect(0, 0, 600, 600);
	genHeader();
	labelHeroSpeciality = std::make_shared<CLabel>(pos.w / 2 + 4, 117, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[78]);
	auto heroIndex = hero.getNum() >= CGI->heroh->size() ? 0 : hero.getNum();

	imageSpeciality = std::make_shared<CAnimImage>(AnimationPath::builtin("UN44"), (*CGI->heroh)[heroIndex]->imageIndex, 0, pos.w / 2 - 22, 134);
	labelSpecialityName = std::make_shared<CLabel>(pos.w / 2, 188, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, (*CGI->heroh)[heroIndex]->getSpecialtyNameTranslated());
}