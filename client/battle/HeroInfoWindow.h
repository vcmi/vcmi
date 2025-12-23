/*
 * HeroInfoWindow.h, part of VCMI engine
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
class CAnimImage;

VCMI_LIB_NAMESPACE_BEGIN
struct InfoAboutHero;
VCMI_LIB_NAMESPACE_END

class HeroInfoBasicPanel : public CIntObject //extracted from InfoWindow to fit better as non-popup embed element
{
private:
	std::shared_ptr<CPicture> background;
	std::vector<std::shared_ptr<CLabel>> labels;
	std::vector<std::shared_ptr<CAnimImage>> icons;

public:
	HeroInfoBasicPanel(const InfoAboutHero & hero, const Point * position, bool initializeBackground = true);

	void show(Canvas & to) override;

	void initializeData(const InfoAboutHero & hero);
	void update(const InfoAboutHero & updatedInfo);
};

class HeroInfoWindow : public CWindowObject
{
private:
	std::shared_ptr<HeroInfoBasicPanel> content;

public:
	HeroInfoWindow(const InfoAboutHero & hero, const Point * position);
};
