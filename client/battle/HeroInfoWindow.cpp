/*
 * HeroInfoWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "HeroInfoWindow.h"

#include "../widgets/Images.h"
#include "../widgets/TextControls.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/gameState/InfoAboutArmy.h"
#include "../../lib/texts/CGeneralTextHandler.h"

HeroInfoBasicPanel::HeroInfoBasicPanel(const InfoAboutHero & hero, const Point * position, bool initializeBackground)
	: CIntObject(0)
{
	OBJECT_CONSTRUCTION;
	if(position != nullptr)
		moveTo(*position);

	if(initializeBackground)
	{
		background = std::make_shared<CPicture>(ImagePath::builtin("CHRPOP"), Rect(1, 1, 76, 200), 1, 1);
		background->setPlayerColor(hero.owner);
	}

	initializeData(hero);
}

void HeroInfoBasicPanel::initializeData(const InfoAboutHero & hero)
{
	OBJECT_CONSTRUCTION;
	auto attack = hero.details->primskills[0];
	auto defense = hero.details->primskills[1];
	auto power = hero.details->primskills[2];
	auto knowledge = hero.details->primskills[3];
	auto morale = hero.details->morale;
	auto luck = hero.details->luck;
	auto currentSpellPoints = hero.details->mana;
	auto maxSpellPoints = hero.details->manaLimit;

	icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), hero.getIconIndex(), 0, 10, 6));

	//primary stats
	labels.push_back(std::make_shared<CLabel>(9, 75, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[380] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 87, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[381] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 99, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[382] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 111, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[383] + ":"));

	labels.push_back(std::make_shared<CLabel>(69, 87, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(attack)));
	labels.push_back(std::make_shared<CLabel>(69, 99, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(defense)));
	labels.push_back(std::make_shared<CLabel>(69, 111, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(power)));
	labels.push_back(std::make_shared<CLabel>(69, 123, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(knowledge)));

	//morale+luck
	labels.push_back(std::make_shared<CLabel>(9, 131, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[384] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 143, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[385] + ":"));

	icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("IMRL22"), std::clamp(morale + 3, 0, 6), 0, 47, 131));
	icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("ILCK22"), std::clamp(luck + 3, 0, 6), 0, 47, 143));

	//spell points
	labels.push_back(std::make_shared<CLabel>(39, 174, EFonts::FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[387]));
	labels.push_back(std::make_shared<CLabel>(39, 186, EFonts::FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, std::to_string(currentSpellPoints) + "/" + std::to_string(maxSpellPoints)));
}

void HeroInfoBasicPanel::update(const InfoAboutHero & updatedInfo)
{
	icons.clear();
	labels.clear();

	initializeData(updatedInfo);
}

void HeroInfoBasicPanel::show(Canvas & to)
{
	showAll(to);
	CIntObject::show(to);
}

HeroInfoWindow::HeroInfoWindow(const InfoAboutHero & hero, const Point * position)
	: CWindowObject(RCLICK_POPUP | SHADOW_DISABLED, ImagePath::builtin("CHRPOP"))
{
	OBJECT_CONSTRUCTION;
	if(position != nullptr)
		moveTo(*position);

	background->setPlayerColor(hero.owner); //maybe add this functionality to base class?

	content = std::make_shared<HeroInfoBasicPanel>(hero, nullptr, false);
}
