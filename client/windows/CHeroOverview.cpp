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
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CHeroHandler.h"

CHeroOverview::CHeroOverview(const HeroTypeID & h)
	: CWindowObject(BORDERED | RCLICK_POPUP), hero { h }
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

    heroIndex = hero.getNum();

	genHeroWindow();

	center();
}

void CHeroOverview::genHeader()
{
	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), pos);
	updateShadow();

    int yOffset = 35;
    int borderOffset = 5;
    int alpha = 75;

    ColorRGBA borderColor = ColorRGBA(128, 100, 75);

    Canvas canvas = Canvas(Point(pos.w, pos.h));

    // hero image
    canvas.drawBorder(Rect(borderOffset - 1, borderOffset + yOffset - 1, 58 + 2, 64 + 2), borderColor);

    // hero name
    canvas.drawColorBlended(Rect(64 + borderOffset, borderOffset + yOffset, 220, 64), ColorRGBA(0, 0, 0, alpha));
    canvas.drawBorder(Rect(64 + borderOffset - 1, borderOffset + yOffset - 1, 220 + 2, 64 + 2), borderColor);

    // vertical line
    canvas.drawLine(Point(295, borderOffset + yOffset - 1), Point(295, 445), borderColor, borderColor);
    
    // skill header
    canvas.drawColorBlended(Rect(borderOffset, 2 * borderOffset + yOffset + 64, 284, 20), ColorRGBA(0, 0, 0, alpha));
    canvas.drawBorder(Rect(borderOffset - 1, 2 * borderOffset + yOffset + 64 - 1, 284 + 2, 20 + 2), borderColor);

    // skill
    for(int i = 0; i < 4; i++)
        canvas.drawBorder(Rect((284 / 4) * i + 21 - 1, 3 * borderOffset + yOffset + 64 + 20 + 1 - 1, 42 + 2, 42 + 2), borderColor);

    // skill footer
    canvas.drawColorBlended(Rect(borderOffset, 4 * borderOffset + yOffset + 64 + 20 + 44, 284, 20), ColorRGBA(0, 0, 0, alpha));
    canvas.drawBorder(Rect(borderOffset - 1, 4 * borderOffset + yOffset + 64 + 20 + 44 - 1, 284 + 2, 20 + 2), borderColor);

    // hero biography
    canvas.drawColorBlended(Rect(borderOffset, 5 * borderOffset + yOffset + 64 + 20 + 44 + 20, 284, 130), ColorRGBA(0, 0, 0, alpha));
    canvas.drawBorder(Rect(borderOffset - 1, 5 * borderOffset + yOffset + 64 + 20 + 44 + 20 - 1, 284 + 2, 130 + 2), borderColor);

    // speciality name
    canvas.drawColorBlended(Rect(2 * borderOffset + 44, 6 * borderOffset + yOffset + 64 + 20 + 44 + 20 + 130, 235, 44), ColorRGBA(0, 0, 0, alpha));
    canvas.drawBorder(Rect(2 * borderOffset + 44 - 1, 6 * borderOffset + yOffset + 64 + 20 + 44 + 20 + 130 - 1, 235 + 2, 44 + 2), borderColor);

    // speciality image
    canvas.drawBorder(Rect(borderOffset - 1, 6 * borderOffset + yOffset + 64 + 20 + 44 + 20 + 130 - 1, 44 + 2, 44 + 2), borderColor);

    // speciality description
    canvas.drawColorBlended(Rect(borderOffset, 7 * borderOffset + yOffset + 64 + 20 + 44 + 20 + 130 + 44, 284, 85), ColorRGBA(0, 0, 0, alpha));
    canvas.drawBorder(Rect(borderOffset - 1, 7 * borderOffset + yOffset + 64 + 20 + 44 + 20 + 130 + 44 - 1, 284 + 2, 85 + 2), borderColor);

    // army title
    canvas.drawColorBlended(Rect(302, borderOffset + yOffset, 292, 30), ColorRGBA(0, 0, 0, alpha));
    canvas.drawBorder(Rect(302 - 1, borderOffset + yOffset - 1, 292 + 2, 30 + 2), borderColor);

    // army
    for(int i = 0; i < 6; i++)
    {
        int space = (292 - 32 - 6 * 32) / 5;
        canvas.drawColorBlended(Rect(302 + i * (32 + space) + 16, 2 * borderOffset + yOffset + 30, 32, 32), ColorRGBA(0, 0, 0, alpha));
        canvas.drawBorder(Rect(302 + i * (32 + space) + 16 - 1, 2 * borderOffset + yOffset + 30 - 1, 32 + 2, 32 + 2), borderColor);
    }

    // army footer
    canvas.drawColorBlended(Rect(302, 3 * borderOffset + yOffset + 30 + 32, 292, 20), ColorRGBA(0, 0, 0, alpha));
    canvas.drawBorder(Rect(302 - 1, 3 * borderOffset + yOffset + 30 + 32 - 1, 292 + 2, 20 + 2), borderColor);

    // warmachine title
    canvas.drawColorBlended(Rect(302, 4 * borderOffset + yOffset + 30 + 32 + 20, 292, 30), ColorRGBA(0, 0, 0, alpha));
    canvas.drawBorder(Rect(302 - 1, 4 * borderOffset + yOffset + 30 + 32 + 20 - 1, 292 + 2, 30 + 2), borderColor);

    // warmachine
    for(int i = 0; i < 6; i++)
    {
        int space = (292 - 32 - 6 * 32) / 5;
        canvas.drawColorBlended(Rect(302 + i * (32 + space) + 16, 5 * borderOffset + yOffset + 30 + 32 + 20 + 30, 32, 32), ColorRGBA(0, 0, 0, alpha));
        canvas.drawBorder(Rect(302 + i * (32 + space) + 16 - 1, 5 * borderOffset + yOffset + 30 + 32 + 20 + 30 - 1, 32 + 2, 32 + 2), borderColor);
    }

    // secskill title
    canvas.drawColorBlended(Rect(302, 6 * borderOffset + yOffset + 30 + 32 + 20 + 30 + 32, (292 / 2) - 2 * borderOffset, 30), ColorRGBA(0, 0, 0, alpha));
    canvas.drawBorder(Rect(302 - 1, 6 * borderOffset + yOffset + 30 + 32 + 20 + 30 + 32 - 1, (292 / 2) - 2 * borderOffset + 2, 30 + 2), borderColor);

    // vertical line
    canvas.drawLine(Point(302 + (292 / 2), 6 * borderOffset + yOffset + 30 + 32 + 20 + 30 + 32), Point(302 + (292 / 2), 445), borderColor, borderColor);

    // spell title
    canvas.drawColorBlended(Rect(302 + (292 / 2) + 2 * borderOffset, 6 * borderOffset + yOffset + 30 + 32 + 20 + 30 + 32, (292 / 2) - 2 * borderOffset, 30), ColorRGBA(0, 0, 0, alpha));
    canvas.drawBorder(Rect(302 + (292 / 2) + 2 * borderOffset - 1, 6 * borderOffset + yOffset + 30 + 32 + 20 + 30 + 32 - 1, (292 / 2) - 2 * borderOffset + 2, 30 + 2), borderColor);

    // secskill
    for(int i = 0; i < 6; i++)
    {
        canvas.drawColorBlended(Rect(302, 7 * borderOffset + yOffset + 30 + 32 + 20 + 30 + 32 + 30 + i * (32 + borderOffset), 32, 32), ColorRGBA(0, 0, 0, alpha));
        canvas.drawBorder(Rect(302 - 1, 7 * borderOffset + yOffset + 30 + 32 + 20 + 30 + 32 + 30 + i * (32 + borderOffset) - 1, 32 + 2, 32 + 2), borderColor);
        canvas.drawColorBlended(Rect(302 + 32 + borderOffset, 7 * borderOffset + yOffset + 30 + 32 + 20 + 30 + 32 + 30 + i * (32 + borderOffset), (292 / 2) - 32 - 3 * borderOffset, 32), ColorRGBA(0, 0, 0, alpha));
        canvas.drawBorder(Rect(302 + 32 + borderOffset - 1, 7 * borderOffset + yOffset + 30 + 32 + 20 + 30 + 32 + 30 + i * (32 + borderOffset) - 1, (292 / 2) - 32 - 3 * borderOffset + 2, 32 + 2), borderColor);
    }

    // spell
    for(int i = 0; i < 6; i++)
    {
        canvas.drawColorBlended(Rect(302 + (292 / 2) + 2 * borderOffset, 7 * borderOffset + yOffset + 30 + 32 + 20 + 30 + 32 + 30 + i * (32 + borderOffset), 32, 32), ColorRGBA(0, 0, 0, alpha));
        canvas.drawBorder(Rect(302 + (292 / 2) + 2 * borderOffset - 1, 7 * borderOffset + yOffset + 30 + 32 + 20 + 30 + 32 + 30 + i * (32 + borderOffset) - 1, 32 + 2, 32 + 2), borderColor);
        canvas.drawColorBlended(Rect(302 + (292 / 2) + 2 * borderOffset + 32 + borderOffset, 7 * borderOffset + yOffset + 30 + 32 + 20 + 30 + 32 + 30 + i * (32 + borderOffset), (292 / 2) - 32 - 3 * borderOffset, 32), ColorRGBA(0, 0, 0, alpha));
        canvas.drawBorder(Rect(302 + (292 / 2) + 2 * borderOffset + 32 + borderOffset - 1, 7 * borderOffset + yOffset + 30 + 32 + 20 + 30 + 32 + 30 + i * (32 + borderOffset) - 1, (292 / 2) - 32 - 3 * borderOffset + 2, 32 + 2), borderColor);
    }

    std::shared_ptr<IImage> backgroundShapesImg = GH.renderHandler().createImage(canvas.getInternalSurface());
    backgroundShapes = std::make_shared<CPicture>(backgroundShapesImg, pos);

	labelTitle = std::make_shared<CLabel>(pos.w / 2 + 8, 21, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[77]);

    // hero image
	imageHero = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), (*CGI->heroh)[heroIndex]->imageIndex, 0, borderOffset, borderOffset + yOffset);

    // hero name
	labelHeroName = std::make_shared<CLabel>(64 + borderOffset + 110, borderOffset + yOffset + 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, (*CGI->heroh)[heroIndex]->getNameTranslated());
	labelHeroClass = std::make_shared<CLabel>(64 + borderOffset + 110, borderOffset + yOffset + 45, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, (*CGI->heroh)[heroIndex]->heroClass->getNameTranslated());

    // skills header
    for(int i = 0; i < 4; i++)
        labelSkillHeader.push_back(std::make_shared<CLabel>((284 / 4) * i + 42, 2 * borderOffset + yOffset + 64 + 10, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->jktexts[1 + i]));

    // skill
    for(int i = 0; i < 4; i++)
        imageSkill.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL42"), (const int[]){0, 1, 2, 5}[i], 0, (284 / 4) * i + 21, 3 * borderOffset + yOffset + 64 + 20 + 1));

    // skills footer
    for(int i = 0; i < 4; i++)
        labelSkillFooter.push_back(std::make_shared<CLabel>((284 / 4) * i + 42, 4 * borderOffset + yOffset + 64 + 20 + 44 + 10, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, std::to_string((*CGI->heroh)[heroIndex]->heroClass->primarySkillInitial[i])));

    // hero biography
    labelHeroBiography = std::make_shared<CMultiLineLabel>(Rect(2 * borderOffset, 5 * borderOffset + borderOffset + yOffset + 64 + 20 + 44 + 20, 284 - 2 * borderOffset, 130 - 2 * borderOffset), FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, (*CGI->heroh)[heroIndex]->getBiographyTranslated());

    // speciality name
	labelHeroSpeciality = std::make_shared<CLabel>(3 * borderOffset + 44, 7 * borderOffset + yOffset + 64 + 20 + 44 + 20 + 130, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[78]);
	labelSpecialityName = std::make_shared<CLabel>(3 * borderOffset + 44, 7 * borderOffset + yOffset + 64 + 20 + 44 + 20 + 130 + 20, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, (*CGI->heroh)[heroIndex]->getSpecialtyNameTranslated());

    // speciality image
    imageSpeciality = std::make_shared<CAnimImage>(AnimationPath::builtin("UN44"), (*CGI->heroh)[heroIndex]->imageIndex, 0, borderOffset, 6 * borderOffset + yOffset + 64 + 20 + 44 + 20 + 130);

    // speciality description
	labelSpecialityDescription = std::make_shared<CMultiLineLabel>(Rect(2 * borderOffset, 8 * borderOffset + yOffset + 64 + 20 + 44 + 20 + 130 + 44 - 1, 284 - borderOffset, 85 - borderOffset), FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, (*CGI->heroh)[heroIndex]->getSpecialtyDescriptionTranslated());

    // army title
    labelArmyTitle = std::make_shared<CLabel>(302 + borderOffset, 2 * borderOffset + yOffset + 2, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, "TODO: Starting Army");

    // army
    int i = 0;
    for(auto & army : (*CGI->heroh)[heroIndex]->initialArmy)
    {
        if((*CGI->creh)[army.creature]->warMachine == ArtifactID::NONE)
        {
            int space = (292 - 32 - 6 * 32) / 5;
            imageArmy.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), (*CGI->creh)[army.creature]->getIconIndex(), 0, 302 + i * (32 + space) + 16, 2 * borderOffset + yOffset + 30));
            labelArmyCount.push_back(std::make_shared<CLabel>(302 + i * (32 + space) + 32, 3 * borderOffset + yOffset + 30 + 32 + 10, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, (army.minAmount == army.maxAmount) ? std::to_string(army.minAmount) : std::to_string(army.minAmount) + "-" + std::to_string(army.maxAmount)));
            i++;
        }
    }


    // war machine title
    labelWarMachineTitle = std::make_shared<CLabel>(302 + borderOffset, 4 * borderOffset + yOffset + 30 + 32 + 28, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, "TODO: War Machine");

    // war machine
    i = 0;
    for(auto & army : (*CGI->heroh)[heroIndex]->initialArmy)
    {
        int space = (292 - 32 - 6 * 32) / 5;
        if(i == 0)
        {
            imageWarMachine.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), (*CGI->creh)[army.creature.CATAPULT]->getIconIndex(), 0, 302 + i * (32 + space) + 16, 5 * borderOffset + yOffset + 30 + 32 + 20 + 30));
            i++;
        }
        if((*CGI->creh)[army.creature]->warMachine != ArtifactID::NONE)
        {
            imageWarMachine.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), (*CGI->creh)[army.creature]->getIconIndex(), 0, 302 + i * (32 + space) + 16, 5 * borderOffset + yOffset + 30 + 32 + 20 + 30));
            i++;
        }
    }

    /*std::shared_ptr<CLabel> labelArmyTitle;
    std::vector<std::shared_ptr<CAnimImage>> imageArmy;
    std::vector<std::shared_ptr<CLabel>> labelArmyNames;

    std::shared_ptr<CLabel> labelWarMachineTitle;
    std::vector<std::shared_ptr<CAnimImage>> imageWarMachine;
    std::vector<std::shared_ptr<CLabel>> labelWarMachineNames;

    std::shared_ptr<CLabel> labelSpellBookTitle;
    std::shared_ptr<CAnimImage> imageSpellBook;
    std::vector<std::shared_ptr<CAnimImage>> imageSpells;
    std::vector<std::shared_ptr<CLabel>> labelSpellsNames;*/
}

void CHeroOverview::genHeroWindow()
{
	pos = Rect(0, 0, 600, 450 + 35);
	genHeader();
}