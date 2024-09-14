/*
 * CPlayerState.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CPlayerState.h"
#include "mapObjects/CGDwelling.h"
#include "mapObjects/CGTownInstance.h"
#include "mapObjects/CGHeroInstance.h"
#include "gameState/QuestInfo.h"
#include "texts/CGeneralTextHandler.h"
#include "VCMI_Lib.h"

VCMI_LIB_NAMESPACE_BEGIN

PlayerState::PlayerState()
 : color(-1), human(false), cheated(false), enteredWinningCheatCode(false),
   enteredLosingCheatCode(false), status(EPlayerStatus::INGAME)
{
	setNodeType(PLAYER);
}

PlayerState::~PlayerState() = default;

std::string PlayerState::nodeName() const
{
	return "Player " + color.toString();
}

PlayerColor PlayerState::getId() const
{
	return color;
}

int32_t PlayerState::getIndex() const
{
	return color.getNum();
}

int32_t PlayerState::getIconIndex() const 
{
	return color.getNum();
}

std::string PlayerState::getJsonKey() const
{
	return color.toString();
}

std::string PlayerState::getModScope() const
{
	return "core";
}

std::string PlayerState::getNameTranslated() const
{
	return VLC->generaltexth->translate(getNameTextID());
}

std::string PlayerState::getNameTextID() const
{
	return TextIdentifier("core.plcolors", color.getNum()).get();
}

void PlayerState::registerIcons(const IconRegistar & cb) const
{
	//We cannot register new icons for players
}

TeamID PlayerState::getTeam() const
{
	return team;
}

bool PlayerState::isHuman() const
{
	return human;
}

const IBonusBearer * PlayerState::getBonusBearer() const
{
	return this;
}

int PlayerState::getResourceAmount(int type) const
{
	return vstd::atOrDefault(resources, static_cast<size_t>(type), 0);
}

template<typename T>
std::vector<T> PlayerState::getObjectsOfType() const
{
	std::vector<T> result;
	for (auto const & object : ownedObjects)
	{
		auto casted = dynamic_cast<T>(object);
		if (casted)
			result.push_back(casted);
	}
	return result;
}

std::vector<const CGHeroInstance *> PlayerState::getHeroes() const
{
	return getObjectsOfType<const CGHeroInstance *>();
}

std::vector<const CGTownInstance *> PlayerState::getTowns() const
{
	return getObjectsOfType<const CGTownInstance *>();
}

std::vector<CGHeroInstance *> PlayerState::getHeroes()
{
	return getObjectsOfType<CGHeroInstance *>();
}

std::vector<CGTownInstance *> PlayerState::getTowns()
{
	return getObjectsOfType<CGTownInstance *>();
}

std::vector<const CGObjectInstance *> PlayerState::getOwnedObjects() const
{
	return {ownedObjects.begin(), ownedObjects.end()};
}

void PlayerState::addOwnedObject(CGObjectInstance * object)
{
	assert(object->asOwnable() != nullptr);
	ownedObjects.push_back(object);
}

void PlayerState::removeOwnedObject(CGObjectInstance * object)
{
	vstd::erase(ownedObjects, object);
}


VCMI_LIB_NAMESPACE_END
