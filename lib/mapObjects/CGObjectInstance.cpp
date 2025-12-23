/*
 * CGObjectInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CGObjectInstance.h"

#include "CGHeroInstance.h"
#include "ObjectTemplate.h"

#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameEventCallback.h"
#include "../gameState/CGameState.h"
#include "../texts/CGeneralTextHandler.h"
#include "../constants/StringConstants.h"
#include "../TerrainHandler.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapping/CMap.h"
#include "../networkPacks/PacksForClient.h"
#include "../serializer/JsonSerializeFormat.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

//TODO: remove constructor
CGObjectInstance::CGObjectInstance(IGameInfoCallback *cb):
	IObjectInterface(cb),
	pos(-1,-1,-1),
	ID(Obj::NO_OBJ),
	subID(-1),
	tempOwner(PlayerColor::UNFLAGGABLE),
	blockVisit(false),
	removable(false)
{
}

//must be instantiated in .cpp file for access to complete types of all member fields
CGObjectInstance::~CGObjectInstance() = default;

MapObjectID CGObjectInstance::getObjGroupIndex() const
{
	return ID;
}

MapObjectSubID CGObjectInstance::getObjTypeIndex() const
{
	return subID;
}

int3 CGObjectInstance::anchorPos() const
{
	return pos;
}

int3 CGObjectInstance::getTopVisiblePos() const
{
	return anchorPos() - appearance->getTopVisibleOffset();
}

void CGObjectInstance::setOwner(const PlayerColor & ow)
{
	tempOwner = ow;
}

void CGObjectInstance::setAnchorPos(int3 newPos)
{
	pos = newPos;
}

int CGObjectInstance::getWidth() const
{
	return appearance->getWidth();
}

int CGObjectInstance::getHeight() const
{
	return appearance->getHeight();
}

bool CGObjectInstance::visitableAt(const int3 & testPos) const
{
	return anchorPos().z == testPos.z && appearance->isVisitableAt(anchorPos().x - testPos.x, anchorPos().y - testPos.y);
}

bool CGObjectInstance::blockingAt(const int3 & testPos) const
{
	return anchorPos().z == testPos.z && appearance->isBlockedAt(anchorPos().x - testPos.x, anchorPos().y - testPos.y);
}

bool CGObjectInstance::coveringAt(const int3 & testPos) const
{
	return anchorPos().z == testPos.z && appearance->isVisibleAt(anchorPos().x - testPos.x, anchorPos().y - testPos.y);
}

std::set<int3> CGObjectInstance::getBlockedPos() const
{
	std::set<int3> ret;
	for(int w=0; w<getWidth(); ++w)
	{
		for(int h=0; h<getHeight(); ++h)
		{
			if(appearance->isBlockedAt(w, h))
				ret.insert(int3(anchorPos().x - w, anchorPos().y - h, anchorPos().z));
		}
	}
	return ret;
}

const std::set<int3> & CGObjectInstance::getBlockedOffsets() const
{
	return appearance->getBlockedOffsets();
}

void CGObjectInstance::setType(MapObjectID newID, MapObjectSubID newSubID)
{
	auto position = visitablePos();
	auto oldOffset = appearance->getCornerOffset();
	const auto * tile = cb->getTile(position);

	//recalculate blockvis tiles - new appearance might have different blockmap than before
	cb->gameState().getMap().hideObject(this);
	auto handler = LIBRARY->objtypeh->getHandlerFor(newID, newSubID);

	if(!handler->getTemplates(tile->getTerrainID()).empty())
	{
		appearance = handler->getTemplates(tile->getTerrainID())[0];
	}
	else
	{
		logGlobal->warn("Object %d:%d at %s has no templates suitable for terrain %s", newID, newSubID, visitablePos().toString(), tile->getTerrain()->getNameTranslated());
		appearance = handler->getTemplates()[0]; // get at least some appearance since alternative is crash
	}

	bool needToAdjustOffset = false;

	// FIXME: potentially unused code - setType is NOT called when releasing hero from prison
	// instead, appearance update & pos adjustment occurs in GiveHero::applyGs
	needToAdjustOffset |= this->ID == Obj::PRISON && newID == Obj::HERO;
	needToAdjustOffset |= newID == Obj::MONSTER;
	needToAdjustOffset |= newID == Obj::CREATURE_GENERATOR1 || newID == Obj::CREATURE_GENERATOR2 || newID == Obj::CREATURE_GENERATOR3 || newID == Obj::CREATURE_GENERATOR4;

	if(needToAdjustOffset)
	{
		// adjust position since object visitable offset might have changed
		auto newOffset = appearance->getCornerOffset();
		pos = pos - oldOffset + newOffset;
	}

	this->ID = Obj(newID);
	this->subID = newSubID;

	cb->gameState().getMap().showObject(this);
}

void CGObjectInstance::pickRandomObject(IGameRandomizer & gameRandomizer)
{
	// no-op
}

void CGObjectInstance::initObj(IGameRandomizer & gameRandomizer)
{
	// no-op
}

void CGObjectInstance::setProperty( ObjProperty what, ObjPropertyID identifier )
{
	setPropertyDer(what, identifier); // call this before any actual changes (needed at least for dwellings)

	switch(what)
	{
	case ObjProperty::OWNER:
		tempOwner = identifier.as<PlayerColor>();
		break;
	case ObjProperty::BLOCKVIS:
		// Never actually used in code, but possible in ERM
		blockVisit = identifier.getNum();
		break;
	case ObjProperty::ID:
		ID = identifier.as<MapObjectID>();
		break;
	}
}

TObjectTypeHandler CGObjectInstance::getObjectHandler() const
{
	return LIBRARY->objtypeh->getHandlerFor(ID, subID);
}

std::string CGObjectInstance::getTypeName() const
{
	return getObjectHandler()->getTypeName();
}

std::string CGObjectInstance::getSubtypeName() const
{
	return getObjectHandler()->getSubTypeName();
}

void CGObjectInstance::setPropertyDer( ObjProperty what, ObjPropertyID identifier )
{}

int3 CGObjectInstance::getSightCenter() const
{
	return visitablePos();
}

int CGObjectInstance::getSightRadius() const
{
	return 3;
}

int3 CGObjectInstance::getVisitableOffset() const
{
	if (!isVisitable())
		logGlobal->debug("Attempt to access visitable offset on a non-visitable object!");
	return appearance->getVisitableOffset();
}

void CGObjectInstance::giveDummyBonus(IGameEventCallback & gameEvents, const ObjectInstanceID & heroID, BonusDuration::Type duration) const
{
	GiveBonus gbonus;
	gbonus.bonus.type = BonusType::NONE;
	gbonus.id = heroID;
	gbonus.bonus.duration = duration;
	gbonus.bonus.source = BonusSource::OBJECT_TYPE;
	gbonus.bonus.sid = BonusSourceID(ID);
	gameEvents.giveHeroBonus(&gbonus);
}

std::string CGObjectInstance::getObjectName() const
{
	return LIBRARY->objtypeh->getObjectName(ID, subID);
}

std::optional<AudioPath> CGObjectInstance::getAmbientSound(vstd::RNG & rng) const
{
	const auto & sounds = LIBRARY->objtypeh->getObjectSounds(ID, subID).ambient;
	if(!sounds.empty())
		return sounds.front(); // TODO: Support randomization of ambient sounds

	return std::nullopt;
}

std::optional<AudioPath> CGObjectInstance::getVisitSound(vstd::RNG & rng) const
{
	const auto & sounds = LIBRARY->objtypeh->getObjectSounds(ID, subID).visit;
	if(!sounds.empty())
		return *RandomGeneratorUtil::nextItem(sounds, rng);

	return std::nullopt;
}

std::optional<AudioPath> CGObjectInstance::getRemovalSound(vstd::RNG & rng) const
{
	const auto & sounds = LIBRARY->objtypeh->getObjectSounds(ID, subID).removal;
	if(!sounds.empty())
		return *RandomGeneratorUtil::nextItem(sounds, rng);

	return std::nullopt;
}

std::string CGObjectInstance::getHoverText(PlayerColor player) const
{
	auto text = getObjectName();
	if (tempOwner.isValidPlayer())
		text += "\n" + LIBRARY->generaltexth->arraytxt[23 + tempOwner.getNum()];
	return text;
}

std::string CGObjectInstance::getHoverText(const CGHeroInstance * hero) const
{
	return getHoverText(hero->tempOwner);
}

std::string CGObjectInstance::getPopupText(PlayerColor player) const
{
	return getHoverText(player);
}
std::string CGObjectInstance::getPopupText(const CGHeroInstance * hero) const
{
	return getHoverText(hero);
}

std::vector<Component> CGObjectInstance::getPopupComponents(PlayerColor player) const
{
	return {};
}

std::vector<Component> CGObjectInstance::getPopupComponents(const CGHeroInstance * hero) const
{
	return getPopupComponents(hero->getOwner());
}

void CGObjectInstance::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	switch(ID.toEnum())
	{
	case Obj::SANCTUARY:
		{
			//You enter the sanctuary and immediately feel as if a great weight has been lifted off your shoulders.  You feel safe here.
			h->showInfoDialog(gameEvents, 114);
		}
		break;
	case Obj::TAVERN:
		{
			gameEvents.showObjectWindow(this, EOpenWindowMode::TAVERN_WINDOW, h, true);
		}
		break;
	}
}

int3 CGObjectInstance::visitablePos() const
{
	if (!isVisitable())
		logGlobal->debug("Attempt to access visitable position on a non-visitable object!");

	return pos - getVisitableOffset();
}

bool CGObjectInstance::isVisitable() const
{
	return appearance->isVisitable();
}

bool CGObjectInstance::isBlockedVisitable() const
{
	// TODO: Read from json
	return blockVisit;
}

bool CGObjectInstance::isRemovable() const
{
	// TODO: Read from json
	return removable;
}

bool CGObjectInstance::isCoastVisitable() const
{
	return false;
}

bool CGObjectInstance::passableFor(PlayerColor color) const
{
	return false;
}

void CGObjectInstance::updateFrom(const JsonNode & data)
{

}

void CGObjectInstance::serializeJson(JsonSerializeFormat & handler)
{
	//only save here, loading is handled by map loader
	if(handler.saving)
	{
		std::string ourTypeName = getTypeName();
		std::string ourSubtypeName = getSubtypeName();

		handler.serializeString("type", ourTypeName);
		handler.serializeString("subtype", ourSubtypeName);
		handler.serializeString("instanceName", instanceName);

		handler.serializeInt("x", pos.x);
		handler.serializeInt("y", pos.y);
		handler.serializeInt("l", pos.z);
		JsonNode app;
		appearance->writeJson(app, false);
		handler.serializeRaw("template", app, std::nullopt);
	}

	{
		auto options = handler.enterStruct("options");
		serializeJsonOptions(handler);
	}
}

void CGObjectInstance::serializeJsonOptions(JsonSerializeFormat & handler)
{
	//nothing here
}

void CGObjectInstance::serializeJsonOwner(JsonSerializeFormat & handler)
{
	handler.serializeId("owner", tempOwner, PlayerColor::NEUTRAL);
}

BattleField CGObjectInstance::getBattlefield() const
{
	return LIBRARY->objtypeh->getHandlerFor(ID, subID)->getBattlefield();
}

const IOwnableObject * CGObjectInstance::asOwnable() const
{
	return nullptr;
}

VCMI_LIB_NAMESPACE_END
