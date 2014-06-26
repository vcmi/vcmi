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

#include "CObjectClassesHandler.h"

using namespace boost::assign;

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

static void showInfoDialog(const CGHeroInstance* h, const ui32 txtID, const ui16 soundID)
{
	const PlayerColor playerID = h->getOwner();
	showInfoDialog(playerID,txtID,soundID);
}

///IObjectInterface
void IObjectInterface::onHeroVisit(const CGHeroInstance * h) const
{}

void IObjectInterface::onHeroLeave(const CGHeroInstance * h) const
{}

void IObjectInterface::newTurn () const
{}

IObjectInterface::~IObjectInterface()
{}

IObjectInterface::IObjectInterface()
{}

void IObjectInterface::initObj()
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
    logGlobal->traceStream() << "\t\tReading resources prices ";
	const JsonNode config2(ResourceID("config/resources.json"));
	for(const JsonNode &price : config2["resources_prices"].Vector())
	{
		resVals.push_back(price.Float());
	}
    logGlobal->traceStream() << "\t\tDone loading resource prices!";
}

PlayerColor CGObjectInstance::getOwner() const
{
	//if (state)
	//	return state->owner;
	//else
		return tempOwner; //won't have owner
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

void CGObjectInstance::setOwner(PlayerColor ow)
{
	tempOwner = ow;
}
int CGObjectInstance::getWidth() const//returns width of object graphic in tiles
{
	return appearance.getWidth();
}
int CGObjectInstance::getHeight() const //returns height of object graphic in tiles
{
	return appearance.getHeight();
}
bool CGObjectInstance::visitableAt(int x, int y) const //returns true if object is visitable at location (x, y) form left top tile of image (x, y in tiles)
{
	return appearance.isVisitableAt(pos.x - x, pos.y - y);
}
bool CGObjectInstance::blockingAt(int x, int y) const
{
	return appearance.isBlockedAt(pos.x - x, pos.y - y);
}

bool CGObjectInstance::coveringAt(int x, int y) const
{
	return appearance.isVisibleAt(pos.x - x, pos.y - y);
}

std::set<int3> CGObjectInstance::getBlockedPos() const
{
	std::set<int3> ret;
	for(int w=0; w<getWidth(); ++w)
	{
		for(int h=0; h<getHeight(); ++h)
		{
			if (appearance.isBlockedAt(w, h))
				ret.insert(int3(pos.x - w, pos.y - h, pos.z));
		}
	}
	return ret;
}

std::set<int3> CGObjectInstance::getBlockedOffsets() const
{
	std::set<int3> ret;
	for(int w=0; w<getWidth(); ++w)
	{
		for(int h=0; h<getHeight(); ++h)
		{
			if (appearance.isBlockedAt(w, h))
				ret.insert(int3(-w, -h, 0));
		}
	}
	return ret;
}

void CGObjectInstance::setType(si32 ID, si32 subID)
{
	const TerrainTile &tile = cb->gameState()->map->getTile(visitablePos());

	this->ID = Obj(ID);
	this->subID = subID;

	//recalculate blockvis tiles - new appearance might have different blockmap than before
	cb->gameState()->map->removeBlockVisTiles(this, true);
	auto handler = VLC->objtypeh->getHandlerFor(ID, subID);
	appearance = handler->getTemplates(tile.terType).at(0);
	cb->gameState()->map->addBlockVisTiles(this);
}

void CGObjectInstance::initObj()
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

int CGObjectInstance::getSightRadious() const
{
	return 3;
}

int3 CGObjectInstance::getVisitableOffset() const
{
	for(int y = 0; y < appearance.getHeight(); y++)
		for (int x = 0; x < appearance.getWidth(); x++)
			if (appearance.isVisitableAt(x, y))
				return int3(x,y,0);

    logGlobal->warnStream() << "Warning: getVisitableOffset called on non-visitable obj!";
	return int3(0,0,0);
}

void CGObjectInstance::giveDummyBonus(ObjectInstanceID heroID, ui8 duration) const
{
	GiveBonus gbonus;
	gbonus.bonus.type = Bonus::NONE;
	gbonus.id = heroID.getNum();
	gbonus.bonus.duration = duration;
	gbonus.bonus.source = Bonus::OBJECT;
	gbonus.bonus.sid = ID;
	cb->giveHeroBonus(&gbonus);
}

std::string CGObjectInstance::getObjectName() const
{
	return VLC->objtypeh->getObjectName(ID);
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
			showInfoDialog(h,114,soundBase::GETPROTECTION);
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
	return appearance.isVisitable();
}

bool CGObjectInstance::passableFor(PlayerColor color) const
{
	return false;
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
		if (const TerrainTile *tile = IObjectInterface::cb->getTile(o->pos + offset, false)) //tile is in the map
		{
			if (tile->terType == ETerrainType::WATER  &&  (!tile->blocked || tile->blockingObjects.front()->ID == 8)) //and is water and is not blocked or is blocked by boat
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
		logGlobal->errorStream() << "Shipyard without water!!! " << o->pos << "\t" << o->id;
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
