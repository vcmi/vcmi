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
#include "GameLibrary.h"
#include "callback/IGameInfoCallback.h"
#include "mapObjects/CGHeroInstance.h"
#include "mapObjects/CGTownInstance.h"
#include "gameState/CGameState.h"
#include "gameState/QuestInfo.h"
#include "texts/CGeneralTextHandler.h"
#include "json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

PlayerState::PlayerState(IGameInfoCallback *cb)
	: CBonusSystemNode(BonusNodeType::PLAYER)
	, GameCallbackHolder(cb)
	, color(-1)
	, human(false)
	, cheated(false)
	, playerLocalSettings(std::make_unique<JsonNode>())
	, enteredWinningCheatCode(false)
	, enteredLosingCheatCode(false)
	, status(EPlayerStatus::INGAME)
{
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
	return LIBRARY->generaltexth->translate(getNameTextID());
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
	return resources[type];
}

template<typename T>
std::vector<T> PlayerState::getObjectsOfType() const
{
	std::vector<T> result;
	for (const ObjectInstanceID & objectID : ownedObjects)
	{
		auto objectPtr = cb->gameState().getObjInstance(objectID);
		auto casted = dynamic_cast<T>(objectPtr);
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
	// optimized due to numerous AI access
	using T = const CGTownInstance *;
	std::vector<T> result;
	for (const ObjectInstanceID & objectID : ownedObjects)
	{
		const auto * objectPtr = cb->gameState().getObjInstance(objectID);
		if (objectPtr->ID != MapObjectID::TOWN)
			continue;

		assert(dynamic_cast<T>(objectPtr) != nullptr);
		auto casted = static_cast<T>(objectPtr);
		result.push_back(casted);
	}
	return result;
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
	return getObjectsOfType<const CGObjectInstance *>();
}

void PlayerState::addOwnedObject(CGObjectInstance * object)
{
	assert(object->asOwnable() != nullptr);
	ownedObjects.push_back(object->id);
}

void PlayerState::removeOwnedObject(CGObjectInstance * object)
{
	vstd::erase(ownedObjects, object->id);
}

VCMI_LIB_NAMESPACE_END
