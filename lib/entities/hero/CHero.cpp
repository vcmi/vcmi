/*
 * CHero.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CCreatureHandler.h"
#include "CHero.h"

#include "../../GameLibrary.h"
#include "../../texts/CGeneralTextHandler.h"

CHero::CHero() = default;
CHero::~CHero() = default;

int32_t CHero::getIndex() const
{
	return ID.getNum();
}

int32_t CHero::getIconIndex() const
{
	return imageIndex;
}

std::string CHero::getJsonKey() const
{
	return modScope + ':' + identifier;
}

std::string CHero::getModScope() const
{
	return modScope;
}

HeroTypeID CHero::getId() const
{
	return ID;
}

std::string CHero::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

std::string CHero::getBiographyTranslated() const
{
	return LIBRARY->generaltexth->translate(getBiographyTextID());
}

std::string CHero::getSpecialtyNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getSpecialtyNameTextID());
}

std::string CHero::getSpecialtyDescriptionTranslated() const
{
	return LIBRARY->generaltexth->translate(getSpecialtyDescriptionTextID());
}

std::string CHero::getSpecialtyTooltipTranslated() const
{
	return LIBRARY->generaltexth->translate(getSpecialtyTooltipTextID());
}

std::string CHero::getNameTextID() const
{
	return TextIdentifier("hero", modScope, identifier, "name").get();
}

std::string CHero::getBiographyTextID() const
{
	return TextIdentifier("hero", modScope, identifier, "biography").get();
}

std::string CHero::getSpecialtyNameTextID() const
{
	return TextIdentifier("hero", modScope, identifier, "specialty", "name").get();
}

std::string CHero::getSpecialtyDescriptionTextID() const
{
	return TextIdentifier("hero", modScope, identifier, "specialty", "description").get();
}

std::string CHero::getSpecialtyTooltipTextID() const
{
	return TextIdentifier("hero", modScope, identifier, "specialty", "tooltip").get();
}

CreatureID CHero::defaultCreature() const
{
	for (const auto	& unit : initialArmy)
	{
		if(!unit.creature.hasValue())
			continue;

		if (unit.creature.toCreature()->warMachine != ArtifactID::NONE)
			continue;

		return unit.creature;
	}

	throw std::runtime_error("Hero " + getJsonKey() + " has no valid army stack!");
}

void CHero::registerIcons(const IconRegistar & cb) const
{
	cb(getIconIndex(), 0, "UN32", iconSpecSmall);
	cb(getIconIndex(), 0, "UN44", iconSpecLarge);
	cb(getIconIndex(), 0, "PORTRAITSLARGE", portraitLarge);
	cb(getIconIndex(), 0, "PORTRAITSSMALL", portraitSmall);
}
