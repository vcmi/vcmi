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

class CHeroOverview : public CWindowObject
{
	const HeroTypeID & hero;
	int heroIndex;

    std::shared_ptr<CFilledTexture> backgroundTexture;
    std::shared_ptr<CPicture> backgroundShapes;
    std::shared_ptr<CLabel> labelTitle;
    std::shared_ptr<CAnimImage> imageHero;
    std::shared_ptr<CLabel> labelHeroName;
    std::shared_ptr<CMultiLineLabel> labelHeroBiography;
    std::shared_ptr<CLabel> labelHeroClass;
    std::shared_ptr<CLabel> labelHeroSpeciality;
    std::shared_ptr<CAnimImage> imageSpeciality;
    std::vector<std::shared_ptr<CLabel>> labelSkillHeader;
    std::vector<std::shared_ptr<CAnimImage>> imageSkill;
    std::vector<std::shared_ptr<CLabel>> labelSkillFooter;
    std::shared_ptr<CLabel> labelSpecialityName;
    std::shared_ptr<CMultiLineLabel> labelSpecialityDescription;

    std::shared_ptr<CLabel> labelArmyTitle;
    std::vector<std::shared_ptr<CAnimImage>> imageArmy;
    std::vector<std::shared_ptr<CLabel>> labelArmyCount;

    std::shared_ptr<CLabel> labelWarMachineTitle;
    std::vector<std::shared_ptr<CAnimImage>> imageWarMachine;

    std::shared_ptr<CLabel> labelSpellBookTitle;
    std::shared_ptr<CAnimImage> imageSpellBook;
    std::vector<std::shared_ptr<CAnimImage>> imageSpells;
    std::vector<std::shared_ptr<CLabel>> labelSpellsNames;

    void genHeader();
    void genHeroWindow();

public:
    CHeroOverview(const HeroTypeID & h);
};