/*
 * CHeroOverview.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"

class CLabel;
class CMultiLineLabel;
class CFilledTexture;
class CAnimImage;
class CComponentBox;
class CTextBox;
class TransparentFilledRectangle;
class SimpleLine;
class CSecSkillPlace;

class CHeroOverview : public CWindowObject
{
	const HeroTypeID & hero;
	int heroIdx;

    const int yOffset = 35;
    const int borderOffset = 5;
    const ColorRGBA rectangleColor = ColorRGBA(0, 0, 0, 75);
    const ColorRGBA borderColor = ColorRGBA(128, 100, 75);
    const ColorRGBA grayedColor = ColorRGBA(158, 130, 105);

    std::shared_ptr<CFilledTexture> backgroundTexture;
    std::vector<std::shared_ptr<TransparentFilledRectangle>> backgroundRectangles;
    std::vector<std::shared_ptr<SimpleLine>> backgroundLines;

    std::shared_ptr<CLabel> labelTitle;
    std::shared_ptr<CAnimImage> imageHero;
    std::shared_ptr<CLabel> labelHeroName;
    std::shared_ptr<CTextBox> labelHeroBiography;
    std::shared_ptr<CLabel> labelHeroClass;
    std::shared_ptr<CLabel> labelHeroSpeciality;
    std::shared_ptr<CAnimImage> imageSpeciality;
    std::vector<std::shared_ptr<CLabel>> labelSkillHeader;
    std::vector<std::shared_ptr<CAnimImage>> imageSkill;
    std::vector<std::shared_ptr<CLabel>> labelSkillFooter;
    std::shared_ptr<CLabel> labelSpecialityName;
    std::shared_ptr<CTextBox> labelSpecialityDescription;

    std::shared_ptr<CLabel> labelArmyTitle;
    std::vector<std::shared_ptr<CAnimImage>> imageArmy;
    std::vector<std::shared_ptr<CLabel>> labelArmyCount;

    std::shared_ptr<CLabel> labelWarMachineTitle;
    std::vector<std::shared_ptr<CAnimImage>> imageWarMachine;

    std::shared_ptr<CLabel> labelSpellTitle;
    std::vector<std::shared_ptr<CAnimImage>> imageSpells;
    std::vector<std::shared_ptr<CMultiLineLabel>> labelSpellsNames;

    std::shared_ptr<CLabel> labelSecSkillTitle;
    std::vector<std::shared_ptr<CSecSkillPlace>> secSkills;
    std::vector<std::shared_ptr<CLabel>> labelSecSkillsNames;

    void genBackground();
    void genControls();

public:
    CHeroOverview(const HeroTypeID & h);
};
