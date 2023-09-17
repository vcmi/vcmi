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
    std::shared_ptr<CAnimImage> image;

    std::shared_ptr<CLabel> labelHeroName;
    std::shared_ptr<CLabel> labelHeroClass;
    std::shared_ptr<CLabel> labelHeroSpeciality;
    std::shared_ptr<CAnimImage> imageSpeciality;
    std::shared_ptr<CLabel> labelSpecialityName;

    std::shared_ptr<CTextBox> textBonusDescription;

    void genHeader();
    void genHeroWindow();

public:
    CHeroOverview(const HeroTypeID & h);
};