/*
 * CHeroClass.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CHeroClass.h"

#include "../faction/CFaction.h"

#include "../../GameLibrary.h"
#include "../../texts/CGeneralTextHandler.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

bool CHeroClass::isMagicHero() const
{
	return affinity == MAGIC;
}

int CHeroClass::tavernProbability(FactionID targetFaction) const
{
	auto it = selectionProbability.find(targetFaction);
	if (it != selectionProbability.end())
		return it->second;
	return 0;
}

EAlignment CHeroClass::getAlignment() const
{
	return faction.toEntity(LIBRARY)->getAlignment();
}

int32_t CHeroClass::getIndex() const
{
	return id.getNum();
}

int32_t CHeroClass::getIconIndex() const
{
	return getIndex();
}

std::string CHeroClass::getJsonKey() const
{
	return modScope + ':' + identifier;
}

std::string CHeroClass::getModScope() const
{
	return modScope;
}

HeroClassID CHeroClass::getId() const
{
	return id;
}

void CHeroClass::registerIcons(const IconRegistar & cb) const
{

}

std::string CHeroClass::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

std::string CHeroClass::getNameTextID() const
{
	return TextIdentifier("heroClass", modScope, identifier, "name").get();
}

void CHeroClass::updateFrom(const JsonNode & data)
{
	//TODO: CHeroClass::updateFrom
}

void CHeroClass::serializeJson(JsonSerializeFormat & handler)
{

}

CHeroClass::CHeroClass():
	faction(0),
	affinity(0),
	defaultTavernChance(0)
{
}

VCMI_LIB_NAMESPACE_END
