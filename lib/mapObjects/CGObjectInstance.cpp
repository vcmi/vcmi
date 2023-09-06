/*
 * CObjectHandler.cpp, part of VCMI engine
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

#include "../gameState/CGameState.h"
#include "../CGeneralTextHandler.h"
#include "../IGameCallback.h"
#include "../NetPacks.h"
#include "../constants/StringConstants.h"
#include "../TerrainHandler.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapping/CMap.h"
#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

//TODO: remove constructor
CGObjectInstance::CGObjectInstance():
	pos(-1,-1,-1),
	ID(Obj::NO_OBJ),
	subID(-1),
	tempOwner(PlayerColor::UNFLAGGABLE),
	blockVisit(false)
{
}

//must be instantiated in .cpp file for access to complete types of all member fields
CGObjectInstance::~CGObjectInstance() = default;

int32_t CGObjectInstance::getObjGroupIndex() const
{
	return ID.num;
}

int32_t CGObjectInstance::getObjTypeIndex() const
{
	return subID;
}

int3 CGObjectInstance::getPosition() const
{
	return pos;
}

int3 CGObjectInstance::getTopVisiblePos() const
{
	return pos - appearance->getTopVisibleOffset();
}

void CGObjectInstance::setOwner(const PlayerColor & ow)
{
	tempOwner = ow;
}
int CGObjectInstance::getWidth() const//returns width of object graphic in tiles
{
	return appearance->getWidth();
}
int CGObjectInstance::getHeight() const //returns height of object graphic in tiles
{
	return appearance->getHeight();
}
bool CGObjectInstance::visitableAt(int x, int y) const //returns true if object is visitable at location (x, y) form left top tile of image (x, y in tiles)
{
	return appearance->isVisitableAt(pos.x - x, pos.y - y);
}
bool CGObjectInstance::blockingAt(int x, int y) const
{
	return appearance->isBlockedAt(pos.x - x, pos.y - y);
}

bool CGObjectInstance::coveringAt(int x, int y) const
{
	return appearance->isVisibleAt(pos.x - x, pos.y - y);
}

std::set<int3> CGObjectInstance::getBlockedPos() const
{
	std::set<int3> ret;
	for(int w=0; w<getWidth(); ++w)
	{
		for(int h=0; h<getHeight(); ++h)
		{
			if(appearance->isBlockedAt(w, h))
				ret.insert(int3(pos.x - w, pos.y - h, pos.z));
		}
	}
	return ret;
}

std::set<int3> CGObjectInstance::getBlockedOffsets() const
{
	return appearance->getBlockedOffsets();
}

void CGObjectInstance::setType(si32 newID, si32 newSubID)
{
	auto position = visitablePos();
	auto oldOffset = getVisitableOffset();
	auto &tile = cb->gameState()->map->getTile(position);

	//recalculate blockvis tiles - new appearance might have different blockmap than before
	cb->gameState()->map->removeBlockVisTiles(this, true);
	auto handler = VLC->objtypeh->getHandlerFor(newID, newSubID);
	if(!handler)
	{
		logGlobal->error("Unknown object type %d:%d at %s", newID, newSubID, visitablePos().toString());
		return;
	}
	if(!handler->getTemplates(tile.terType->getId()).empty())
	{
		appearance = handler->getTemplates(tile.terType->getId())[0];
	}
	else
	{
		logGlobal->warn("Object %d:%d at %s has no templates suitable for terrain %s", newID, newSubID, visitablePos().toString(), tile.terType->getNameTranslated());
		appearance = handler->getTemplates()[0]; // get at least some appearance since alternative is crash
	}

	bool needToAdjustOffset = false;

	// FIXME: potentially unused code - setType is NOT called when releasing hero from prison
	// instead, appearance update & pos adjustment occurs in GiveHero::applyGs
	needToAdjustOffset |= this->ID == Obj::PRISON && newID == Obj::HERO;
	needToAdjustOffset |= newID == Obj::MONSTER;

	if(needToAdjustOffset)
	{
		// adjust position since object visitable offset might have changed
		auto newOffset = getVisitableOffset();
		pos = pos - oldOffset + newOffset;
	}

	this->ID = Obj(newID);
	this->subID = newSubID;

	cb->gameState()->map->addBlockVisTiles(this);
}

void CGObjectInstance::initObj(CRandomGenerator & rand)
{
	switch(ID)
	{
	case Obj::TAVERN:
		blockVisit = true;
		break;
	}
}

void CGObjectInstance::setProperty( ui8 what, ui32 val )
{
	setPropertyDer(what, val); // call this before any actual changes (needed at least for dwellings)

	switch(what)
	{
	case ObjProperty::OWNER:
		tempOwner = PlayerColor(val);
		break;
	case ObjProperty::BLOCKVIS:
		blockVisit = val;
		break;
	case ObjProperty::ID:
		ID = Obj(val);
		break;
	case ObjProperty::SUBID:
		subID = val;
		break;
	}
}

void CGObjectInstance::setPropertyDer( ui8 what, ui32 val )
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
	return appearance->getVisitableOffset();
}

void CGObjectInstance::giveDummyBonus(const ObjectInstanceID & heroID, BonusDuration::Type duration) const
{
	GiveBonus gbonus;
	gbonus.bonus.type = BonusType::NONE;
	gbonus.id = heroID.getNum();
	gbonus.bonus.duration = duration;
	gbonus.bonus.source = BonusSource::OBJECT;
	gbonus.bonus.sid = ID;
	cb->giveHeroBonus(&gbonus);
}

std::string CGObjectInstance::getObjectName() const
{
	return VLC->objtypeh->getObjectName(ID, subID);
}

std::optional<std::string> CGObjectInstance::getAmbientSound() const
{
	const auto & sounds = VLC->objtypeh->getObjectSounds(ID, subID).ambient;
	if(!sounds.empty())
		return sounds.front(); // TODO: Support randomization of ambient sounds

	return std::nullopt;
}

std::optional<std::string> CGObjectInstance::getVisitSound() const
{
	const auto & sounds = VLC->objtypeh->getObjectSounds(ID, subID).visit;
	if(!sounds.empty())
		return *RandomGeneratorUtil::nextItem(sounds, CRandomGenerator::getDefault());

	return std::nullopt;
}

std::optional<std::string> CGObjectInstance::getRemovalSound() const
{
	const auto & sounds = VLC->objtypeh->getObjectSounds(ID, subID).removal;
	if(!sounds.empty())
		return *RandomGeneratorUtil::nextItem(sounds, CRandomGenerator::getDefault());

	return std::nullopt;
}

std::string CGObjectInstance::getHoverText(PlayerColor player) const
{
	auto text = getObjectName();
	if (tempOwner.isValidPlayer())
		text += "\n" + VLC->generaltexth->arraytxt[23 + tempOwner.getNum()];
	return text;
}

std::string CGObjectInstance::getHoverText(const CGHeroInstance * hero) const
{
	return getHoverText(hero->tempOwner);
}

void CGObjectInstance::onHeroVisit( const CGHeroInstance * h ) const
{
	switch(ID)
	{
	case Obj::SANCTUARY:
		{
			//You enter the sanctuary and immediately feel as if a great weight has been lifted off your shoulders.  You feel safe here.
			h->showInfoDialog(114);
		}
		break;
	case Obj::TAVERN:
		{
			openWindow(EOpenWindowMode::TAVERN_WINDOW,h->id.getNum(),id.getNum());
		}
		break;
	}
}

int3 CGObjectInstance::visitablePos() const
{
	return pos - getVisitableOffset();
}

bool CGObjectInstance::isVisitable() const
{
	return appearance->isVisitable();
}

bool CGObjectInstance::isBlockedVisitable() const
{
	return blockVisit;
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
		handler.serializeString("type", typeName);
		handler.serializeString("subtype", subTypeName);

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

void CGObjectInstance::afterAddToMap(CMap * map)
{
	//nothing here
}

void CGObjectInstance::afterRemoveFromMap(CMap * map)
{
	//nothing here
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
	return VLC->objtypeh->getHandlerFor(ID, subID)->getBattlefield();
}

VCMI_LIB_NAMESPACE_END
