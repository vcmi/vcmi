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
#include "../widgets/GraphicalPrimitiveCanvas.h"

#include "../../lib/GameSettings.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/spells/CSpellHandler.h"

CHeroOverview::CHeroOverview(const HeroTypeID & h)
	: CWindowObject(BORDERED | RCLICK_POPUP), hero { h }
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

    heroIdx = hero.getNum();

	pos = Rect(0, 0, 600, 485);

	genBackground();
    genControls();

	center();
}

void CHeroOverview::genBackground()
{
	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), pos);
	updateShadow();
}

void CHeroOverview::genControls()
{
    Rect r = Rect();

	labelTitle = std::make_shared<CLabel>(pos.w / 2 + 8, 21, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[77]);

    // hero image
    r = Rect(borderOffset, borderOffset + yOffset, 58, 64);
    backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
	imageHero = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), (*CGI->heroh)[heroIdx]->imageIndex, 0, r.x, r.y);

    // hero name
    r = Rect(64 + borderOffset, borderOffset + yOffset, 220, 64);
    backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
	labelHeroName = std::make_shared<CLabel>(r.x + 110, r.y + 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, (*CGI->heroh)[heroIdx]->getNameTranslated());
	labelHeroClass = std::make_shared<CLabel>(r.x + 110, r.y + 45, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, (*CGI->heroh)[heroIdx]->heroClass->getNameTranslated());

    // vertical line
    backgroundLines.push_back(std::make_shared<SimpleLine>(Point(295, borderOffset + yOffset - 1), Point(295, borderOffset + yOffset - 2 + 439), borderColor));
    
    // skills header
    r = Rect(borderOffset, 2 * borderOffset + yOffset + 64, 284, 20);
    backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
    for(int i = 0; i < 4; i++)
        labelSkillHeader.push_back(std::make_shared<CLabel>((r.w / 4) * i + 42, r.y + 10, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->jktexts[1 + i]));

    // skill
    const int tmp[] = {0, 1, 2, 5};
    for(int i = 0; i < 4; i++)
    {
        r = Rect((284 / 4) * i + 21, 3 * borderOffset + yOffset + 85, 42, 42);
        backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
        imageSkill.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL42"), tmp[i], 0, r.x, r.y));
    }

    // skills footer
    r = Rect(borderOffset, 4 * borderOffset + yOffset + 128, 284, 20);
    backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
    for(int i = 0; i < 4; i++)
    {
        r = Rect((284 / 4) * i + 42, r.y, r.w, r.h);
        labelSkillFooter.push_back(std::make_shared<CLabel>(r.x, r.y + 10, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, std::to_string((*CGI->heroh)[heroIdx]->heroClass->primarySkillInitial[i])));
    } 

    // hero biography
    r = Rect(borderOffset, 5 * borderOffset + yOffset + 148, 284, 130);
    backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
    labelHeroBiography = std::make_shared<CMultiLineLabel>(r.resize(-borderOffset), FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, (*CGI->heroh)[heroIdx]->getBiographyTranslated());

    // speciality name
    r = Rect(2 * borderOffset + 44, 6 * borderOffset + yOffset + 278, 235, 44);
    backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
	labelHeroSpeciality = std::make_shared<CLabel>(r.x + borderOffset, r.y + borderOffset, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[78]);
	labelSpecialityName = std::make_shared<CLabel>(r.x + borderOffset, r.y + borderOffset + 20, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, (*CGI->heroh)[heroIdx]->getSpecialtyNameTranslated());

    // speciality image
    r = Rect(borderOffset, 6 * borderOffset + yOffset + 278, 44, 44);
    backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
    imageSpeciality = std::make_shared<CAnimImage>(AnimationPath::builtin("UN44"), (*CGI->heroh)[heroIdx]->imageIndex, 0, r.x, r.y);

    // speciality description
    r = Rect(borderOffset, 7 * borderOffset + yOffset + 322, 284, 85);
    backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
	labelSpecialityDescription = std::make_shared<CMultiLineLabel>(r.resize(-borderOffset), FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, (*CGI->heroh)[heroIdx]->getSpecialtyDescriptionTranslated());

    // army title
    r = Rect(302, borderOffset + yOffset, 292, 30);
    backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
    labelArmyTitle = std::make_shared<CLabel>(r.x + borderOffset, r.y + borderOffset + 2, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->translate("vcmi.heroOverview.startingArmy"));

    // army numbers
    r = Rect(302, 3 * borderOffset + yOffset + 62, 292, 32);
    backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));

    auto stacksCountChances = VLC->settings()->getVector(EGameSettings::HEROES_STARTING_STACKS_CHANCES);

    // army
    int space = (260 - 7 * 32) / 6;
    for(int i = 0; i < 7; i++)
    {
        r = Rect(318 + i * (32 + space), 2 * borderOffset + yOffset + 30, 32, 32);
        backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
    }
    int i = 0;
    int iStack = 0;
    for(auto & army : (*CGI->heroh)[heroIdx]->initialArmy)
    {
        if((*CGI->creh)[army.creature]->warMachine == ArtifactID::NONE)
        {
            imageArmy.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), (*CGI->creh)[army.creature]->getIconIndex(), 0, 302 + i * (32 + space) + 16, 2 * borderOffset + yOffset + 30));
            labelArmyCount.push_back(std::make_shared<CLabel>(302 + i * (32 + space) + 32, 3 * borderOffset + yOffset + 72, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, (army.minAmount == army.maxAmount) ? std::to_string(army.minAmount) : std::to_string(army.minAmount) + "-" + std::to_string(army.maxAmount)));
            if(iStack<stacksCountChances.size())
                labelArmyCount.push_back(std::make_shared<CLabel>(302 + i * (32 + space) + 32, 3 * borderOffset + yOffset + 86, FONT_SMALL, ETextAlignment::CENTER, grayedColor, std::to_string(stacksCountChances[iStack]) + "%"));
            i++;
        }
        iStack++;
    }

    // war machine title
    r = Rect(302, 4 * borderOffset + yOffset + 94, 292, 30);
    backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
    labelWarMachineTitle = std::make_shared<CLabel>(r.x + borderOffset, r.y + borderOffset + 2, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->translate("vcmi.heroOverview.warMachine"));

    // war machine
    space = (260 - 4 * 32) / 3;
    for(int i = 0; i < 4; i++)
    {
        r = Rect(318 + i * (32 + space), 5 * borderOffset + yOffset + 124, 32, 32);
        backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
    }
    i = 0;
    iStack = 0;
    for(auto & army : (*CGI->heroh)[heroIdx]->initialArmy)
    {
        if(i == 0)
        {
            imageWarMachine.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), (*CGI->creh)[army.creature.CATAPULT]->getIconIndex(), 0, 302 + i * (32 + space) + 16, 5 * borderOffset + yOffset + 124));
            labelArmyCount.push_back(std::make_shared<CLabel>(302 + i * (32 + space) + 51, 5 * borderOffset + yOffset + 144, FONT_SMALL, ETextAlignment::TOPLEFT, grayedColor, "100%"));
            i++;
        }
        if((*CGI->creh)[army.creature]->warMachine != ArtifactID::NONE)
        {
            imageWarMachine.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), (*CGI->creh)[army.creature]->getIconIndex(), 0, 302 + i * (32 + space) + 16, 5 * borderOffset + yOffset + 124));
            if(iStack<stacksCountChances.size())
                labelArmyCount.push_back(std::make_shared<CLabel>(302 + i * (32 + space) + 51, 5 * borderOffset + yOffset + 144, FONT_SMALL, ETextAlignment::TOPLEFT, grayedColor, std::to_string(stacksCountChances[iStack]) + "%"));
            i++;
        }
        iStack++;
    }

    // secskill title
    r = Rect(302, 6 * borderOffset + yOffset + 156, (292 / 2) - 2 * borderOffset, 30);
    backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
    labelSecSkillTitle = std::make_shared<CLabel>(r.x + borderOffset, r.y + borderOffset + 2, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->translate("vcmi.heroOverview.secondarySkills"));

    // vertical line
    backgroundLines.push_back(std::make_shared<SimpleLine>(Point(302 + (292 / 2), 6 * borderOffset + yOffset + 156 - 1), Point(302 + (292 / 2), 6 * borderOffset + yOffset + 156 - 2 + 254), borderColor));

    // spell title
    r = Rect(302 + (292 / 2) + 2 * borderOffset, 6 * borderOffset + yOffset + 156, (292 / 2) - 2 * borderOffset, 30);
    backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
    labelSpellTitle = std::make_shared<CLabel>(r.x + borderOffset, r.y + borderOffset + 2, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->translate("vcmi.heroOverview.spells"));

    // secskill
    for(int i = 0; i < 6; i++)
    {
        r = Rect(302, 7 * borderOffset + yOffset + 186 + i * (32 + borderOffset), 32, 32);
        backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
        r = Rect(r.x + 32 + borderOffset, r.y, (292 / 2) - 32 - 3 * borderOffset, r.h);
        backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
    }
    i = 0;
    for(auto & skill : (*CGI->heroh)[heroIdx]->secSkillsInit)
    {
        imageSecSkills.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("SECSK32"), (*CGI->skillh)[skill.first]->getIconIndex() * 3 + skill.second + 2, 0, 302, 7 * borderOffset + yOffset + 186 + i * (32 + borderOffset)));
        labelSecSkillsNames.push_back(std::make_shared<CLabel>(334 + 2 * borderOffset, 8 * borderOffset + yOffset + 186 + i * (32 + borderOffset) - 5, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->levels[skill.second - 1]));
        labelSecSkillsNames.push_back(std::make_shared<CLabel>(334 + 2 * borderOffset, 8 * borderOffset + yOffset + 186 + i * (32 + borderOffset) + 10, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, (*CGI->skillh)[skill.first]->getNameTranslated()));
        i++;
    }

    // spell
    for(int i = 0; i < 6; i++)
    {
        r = Rect(302 + (292 / 2) + 2 * borderOffset, 7 * borderOffset + yOffset + 186 + i * (32 + borderOffset), 32, 32);
        backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
        r = Rect(r.x + 32 + borderOffset, r.y, (292 / 2) - 32 - 3 * borderOffset, r.h);
        backgroundRectangles.push_back(std::make_shared<TransparentFilledRectangle>(r.resize(1), rectangleColor, borderColor));
    }
    i = 0;
    for(auto & spell : (*CGI->heroh)[heroIdx]->spells)
    {
        if(i == 0)
        {
            if((*CGI->heroh)[heroIdx]->haveSpellBook)
            {
                imageSpells.push_back(std::make_shared<CAnimImage>(GH.renderHandler().loadAnimation(AnimationPath::builtin("ARTIFACT")), 0, Rect(302 + (292 / 2) + 2 * borderOffset, 7 * borderOffset + yOffset + 186 + i * (32 + borderOffset), 32, 32), 0));
            }
            i++;
        }

        imageSpells.push_back(std::make_shared<CAnimImage>(GH.renderHandler().loadAnimation(AnimationPath::builtin("SPELLBON")), (*CGI->spellh)[spell]->getIconIndex(), Rect(302 + (292 / 2) + 2 * borderOffset, 7 * borderOffset + yOffset + 186 + i * (32 + borderOffset), 32, 32), 0));
        labelSpellsNames.push_back(std::make_shared<CLabel>(302 + (292 / 2) + 3 * borderOffset + 32 + borderOffset, 8 * borderOffset + yOffset + 186 + i * (32 + borderOffset) + 3, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, (*CGI->spellh)[spell]->getNameTranslated()));
        i++;
    }
}
