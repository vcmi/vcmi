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
#include "CObjectHandler.h"

#include "../NetPacks.h"
#include "../CGeneralTextHandler.h"
#include "../CHeroHandler.h"
#include "../CSoundBase.h"
#include "../filesystem/ResourceID.h"
#include "../IGameCallback.h"
#include "../CGameState.h"
#include "../StringConstants.h"
#include "../mapping/CMap.h"
#include "../TerrainHandler.h"

#include "CObjectClassesHandler.h"
#include "CGTownInstance.h"

#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

IGameCallback * IObjectInterface::cb = nullptr;

///helpers
static void openWindow(const OpenWindow::EWindow type, const int id1, const int id2 = -1)
{
	OpenWindow ow;
	ow.window = type;
	ow.id1 = id1;
	ow.id2 = id2;
	IObjectInterface::cb->sendAndApply(&ow);
}

static void showInfoDialog(const PlayerColor playerID, const ui32 txtID, const ui16 soundID)
{
	InfoWindow iw;
	iw.soundID = soundID;
	iw.player = playerID;
	iw.text.addTxt(MetaString::ADVOB_TXT,txtID);
	IObjectInterface::cb->sendAndApply(&iw);
}

/*static void showInfoDialog(const ObjectInstanceID heroID, const ui32 txtID, const ui16 soundID)
{
	const PlayerColor playerID = IObjectInterface::cb->getOwner(heroID);
	showInfoDialog(playerID,txtID,soundID);
}*/

static void showInfoDialog(const CGHeroInstance* h, const ui32 txtID, const ui16 soundID = 0)
{
	const PlayerColor playerID = h->getOwner();
	showInfoDialog(playerID,txtID,soundID);
}

///IObjectInterface
void IObjectInterface::onHeroVisit(const CGHeroInstance * h) const
{}

void IObjectInterface::onHeroLeave(const CGHeroInstance * h) const
{}

void IObjectInterface::newTurn(CRandomGenerator & rand) const
{}

IObjectInterface::~IObjectInterface()
{}

IObjectInterface::IObjectInterface()
{}

void IObjectInterface::initObj(CRandomGenerator & rand)
{}

void IObjectInterface::setProperty( ui8 what, ui32 val )
{}

bool IObjectInterface::wasVisited (PlayerColor player) const
{
	return false;
}
bool IObjectInterface::wasVisited (const CGHeroInstance * h) const
{
	return false;
}

void IObjectInterface::postInit()
{}

void IObjectInterface::preInit()
{}

void IObjectInterface::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{}

void IObjectInterface::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{}

void IObjectInterface::garrisonDialogClosed(const CGHeroInstance *hero) const
{}

void IObjectInterface::heroLevelUpDone(const CGHeroInstance *hero) const
{}

CObjectHandler::CObjectHandler()
{
	logGlobal->trace("\t\tReading resources prices ");
	const JsonNode config2(ResourceID("config/resources.json"));
	for(const JsonNode &price : config2["resources_prices"].Vector())
	{
		resVals.push_back(static_cast<ui32>(price.Float()));
	}
	logGlobal->trace("\t\tDone loading resource prices!");
}

CGObjectInstance::CGObjectInstance():
	pos(-1,-1,-1),
	ID(Obj::NO_OBJ),
	subID(-1),
	tempOwner(PlayerColor::UNFLAGGABLE),
	blockVisit(false)
{
}
CGObjectInstance::~CGObjectInstance()
{
}

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

void CGObjectInstance::setOwner(PlayerColor ow)
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

void CGObjectInstance::setType(si32 ID, si32 subID)
{
	auto position = visitablePos();
	auto oldOffset = getVisitableOffset();
	auto &tile = cb->gameState()->map->getTile(position);

	//recalculate blockvis tiles - new appearance might have different blockmap than before
	cb->gameState()->map->removeBlockVisTiles(this, true);
	auto handler = VLC->objtypeh->getHandlerFor(ID, subID);
	if(!handler)
	{
		logGlobal->error("Unknown object type %d:%d at %s", ID, subID, visitablePos().toString());
		return;
	}
	if(!handler->getTemplates(tile.terType->getId()).empty())
	{
		appearance = handler->getTemplates(tile.terType->getId())[0];
	}
	else
	{
		logGlobal->warn("Object %d:%d at %s has no templates suitable for terrain %s", ID, subID, visitablePos().toString(), tile.terType->getNameTranslated());
		appearance = handler->getTemplates()[0]; // get at least some appearance since alternative is crash
	}

	if(this->ID == Obj::PRISON && ID == Obj::HERO)
	{
		auto newOffset = getVisitableOffset();
		// FIXME: potentially unused code - setType is NOT called when releasing hero from prison
		// instead, appearance update & pos adjustment occurs in GiveHero::applyGs

		// adjust position since hero and prison may have different visitable offset
		pos = pos - oldOffset + newOffset;
	}

	this->ID = Obj(ID);
	this->subID = subID;

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

void CGObjectInstance::giveDummyBonus(ObjectInstanceID heroID, ui8 duration) const
{
	GiveBonus gbonus;
	gbonus.bonus.type = Bonus::NONE;
	gbonus.id = heroID.getNum();
	gbonus.bonus.duration = (Bonus::BonusDuration)duration;
	gbonus.bonus.source = Bonus::OBJECT;
	gbonus.bonus.sid = ID;
	cb->giveHeroBonus(&gbonus);
}

std::string CGObjectInstance::getObjectName() const
{
	return VLC->objtypeh->getObjectName(ID, subID);
}

boost::optional<std::string> CGObjectInstance::getAmbientSound() const
{
	const auto & sounds = VLC->objtypeh->getObjectSounds(ID, subID).ambient;
	if(sounds.size())
		return sounds.front(); // TODO: Support randomization of ambient sounds

	return boost::none;
}

boost::optional<std::string> CGObjectInstance::getVisitSound() const
{
	const auto & sounds = VLC->objtypeh->getObjectSounds(ID, subID).visit;
	if(sounds.size())
		return *RandomGeneratorUtil::nextItem(sounds, CRandomGenerator::getDefault());

	return boost::none;
}

boost::optional<std::string> CGObjectInstance::getRemovalSound() const
{
	const auto & sounds = VLC->objtypeh->getObjectSounds(ID, subID).removal;
	if(sounds.size())
		return *RandomGeneratorUtil::nextItem(sounds, CRandomGenerator::getDefault());

	return boost::none;
}

std::string CGObjectInstance::getHoverText(PlayerColor player) const
{
	return getObjectName();
}

std::string CGObjectInstance::getHoverText(const CGHeroInstance * hero) const
{
	return getHoverText(hero->tempOwner);
}

void CGObjectInstance::onHeroVisit( const CGHeroInstance * h ) const
{
	switch(ID)
	{
	case Obj::HILL_FORT:
		{
			openWindow(OpenWindow::HILL_FORT_WINDOW,id.getNum(),h->id.getNum());
		}
		break;
	case Obj::SANCTUARY:
		{
			//You enter the sanctuary and immediately feel as if a great weight has been lifted off your shoulders.  You feel safe here.
			showInfoDialog(h, 114);
		}
		break;
	case Obj::TAVERN:
		{
			openWindow(OpenWindow::TAVERN_WINDOW,h->id.getNum(),id.getNum());
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
		handler.serializeRaw("template",app, boost::none);
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
	ui8 temp = tempOwner.getNum();

	handler.serializeEnum("owner", temp, PlayerColor::NEUTRAL.getNum(), GameConstants::PLAYER_COLOR_NAMES);

	if(!handler.saving)
		tempOwner = PlayerColor(temp);
}

BattleField CGObjectInstance::getBattlefield() const
{
	return VLC->objtypeh->getHandlerFor(ID, subID)->getBattlefield();
}

CGObjectInstanceBySubIdFinder::CGObjectInstanceBySubIdFinder(CGObjectInstance * obj) : obj(obj)
{

}

bool CGObjectInstanceBySubIdFinder::operator()(CGObjectInstance * obj) const
{
	return this->obj->subID == obj->subID;
}

int3 IBoatGenerator::bestLocation() const
{
	std::vector<int3> offsets;
	getOutOffsets(offsets);

	for (auto & offset : offsets)
	{
		if(const TerrainTile *tile = IObjectInterface::cb->getTile(o->pos + offset, false)) //tile is in the map
		{
			if(tile->terType->isWater()  &&  (!tile->blocked || tile->blockingObjects.front()->ID == Obj::BOAT)) //and is water and is not blocked or is blocked by boat
				return o->pos + offset;
		}
	}
	return int3 (-1,-1,-1);
}

IBoatGenerator::EGeneratorState IBoatGenerator::shipyardStatus() const
{
	int3 tile = bestLocation();
	const TerrainTile *t = IObjectInterface::cb->getTile(tile);
	if(!t)
		return TILE_BLOCKED; //no available water
	else if(!t->blockingObjects.size())
		return GOOD; //OK
	else if(t->blockingObjects.front()->ID == Obj::BOAT)
		return BOAT_ALREADY_BUILT; //blocked with boat
	else
		return TILE_BLOCKED; //blocked
}

int IBoatGenerator::getBoatType() const
{
	//We make good ships by default
	return 1;
}


IBoatGenerator::IBoatGenerator(const CGObjectInstance *O)
: o(O)
{
}

void IBoatGenerator::getProblemText(MetaString &out, const CGHeroInstance *visitor) const
{
	switch(shipyardStatus())
	{
	case BOAT_ALREADY_BUILT:
		out.addTxt(MetaString::GENERAL_TXT, 51);
		break;
	case TILE_BLOCKED:
		if(visitor)
		{
			out.addTxt(MetaString::GENERAL_TXT, 134);
			out.addReplacement(visitor->name);
		}
		else
			out.addTxt(MetaString::ADVOB_TXT, 189);
		break;
	case NO_WATER:
		logGlobal->error("Shipyard without water! %s \t %d", o->pos.toString(), o->id.getNum());
		return;
	}
}

void IShipyard::getBoatCost( std::vector<si32> &cost ) const
{
	cost.resize(GameConstants::RESOURCE_QUANTITY);
	cost[Res::WOOD] = 10;
	cost[Res::GOLD] = 1000;
}

IShipyard::IShipyard(const CGObjectInstance *O)
	: IBoatGenerator(O)
{
}

IShipyard * IShipyard::castFrom( CGObjectInstance *obj )
{
	if(!obj)
		return nullptr;

	if(obj->ID == Obj::TOWN)
	{
		return static_cast<CGTownInstance*>(obj);
	}
	else if(obj->ID == Obj::SHIPYARD)
	{
		return static_cast<CGShipyard*>(obj);
	}
	else
	{
		return nullptr;
	}
}

const IShipyard * IShipyard::castFrom( const CGObjectInstance *obj )
{
	return castFrom(const_cast<CGObjectInstance*>(obj));
}

VCMI_LIB_NAMESPACE_END
