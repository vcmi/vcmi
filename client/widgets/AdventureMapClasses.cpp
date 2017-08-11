/*
 * AdventureMapClasses.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AdventureMapClasses.h"

#include <SDL.h>

#include "MiscWidgets.h"
#include "CComponent.h"

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../CPreGame.h"
#include "../Graphics.h"
#include "../CMessage.h"

#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Pixels.h"

#include "../widgets/Images.h"

#include "../windows/InfoWindows.h"
#include "../windows/CAdvmapInterface.h"
#include "../windows/GUIClasses.h"

#include "../battle/CBattleInterfaceClasses.h"
#include "../battle/CBattleInterface.h"

#include "../../CCallback.h"
#include "../../lib/StartInfo.h"
#include "../../lib/CGameState.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CModHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/JsonNode.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/NetPacksBase.h"
#include "../../lib/StringConstants.h"

CList::CListItem::CListItem(CList * Parent):
	CIntObject(LCLICK | RCLICK | HOVER),
	parent(Parent),
	selection(nullptr)
{
}

CList::CListItem::~CListItem()
{
	// select() method in this was already destroyed so we can't safely call method in parent
	if (parent->selected == this)
		parent->selected = nullptr;
}

void CList::CListItem::clickRight(tribool down, bool previousState)
{
	if (down == true)
		showTooltip();
}

void CList::CListItem::clickLeft(tribool down, bool previousState)
{
	if (down == true)
	{
		//second click on already selected item
		if (parent->selected == this)
			open();
		else
		{
			//first click - switch selection
			parent->select(this);
		}
	}
}

void CList::CListItem::hover(bool on)
{
	if (on)
		GH.statusbar->setText(getHoverText());
	else
		GH.statusbar->clear();
}

void CList::CListItem::onSelect(bool on)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	vstd::clear_pointer(selection);
	if (on)
		selection = genSelection();
	select(on);
	GH.totalRedraw();
}

CList::CList(int Size, Point position, std::string btnUp, std::string btnDown, size_t listAmount,
			 int helpUp, int helpDown, CListBox::CreateFunc create, CListBox::DestroyFunc destroy):
	CIntObject(0, position),
	size(Size),
	selected(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	scrollUp = new CButton(Point(0, 0), btnUp, CGI->generaltexth->zelp[helpUp]);
	list = new CListBox(create, destroy, Point(1,scrollUp->pos.h), Point(0, 32), size, listAmount);

	//assign callback only after list was created
	scrollUp->addCallback(std::bind(&CListBox::moveToPrev, list));
	scrollDown = new CButton(Point(0, scrollUp->pos.h + 32*size), btnDown, CGI->generaltexth->zelp[helpDown], std::bind(&CListBox::moveToNext, list));

	scrollDown->addCallback(std::bind(&CList::update, this));
	scrollUp->addCallback(std::bind(&CList::update, this));

	update();
}

void CList::update()
{
	bool onTop = list->getPos() == 0;
	bool onBottom = list->getPos() + size >= list->size();

	scrollUp->block(onTop);
	scrollDown->block(onBottom);
}

void CList::select(CListItem *which)
{
	if (selected == which)
		return;

	if (selected)
		selected->onSelect(false);

	selected = which;
	if (which)
	{
		which->onSelect(true);
		onSelect();
	}
}

int CList::getSelectedIndex()
{
	return list->getIndexOf(selected);
}

void CList::selectIndex(int which)
{
	if (which < 0)
	{
		if (selected)
			select(nullptr);
	}
	else
	{
		list->scrollTo(which);
		update();
		select(dynamic_cast<CListItem*>(list->getItem(which)));
	}
}

void CList::selectNext()
{
	int index = getSelectedIndex() + 1;
	if (index >= list->size())
		index = 0;
	selectIndex(index);
}

void CList::selectPrev()
{
	int index = getSelectedIndex();
	if (index <= 0)
		selectIndex(0);
	else
		selectIndex(index-1);
}

CHeroList::CEmptyHeroItem::CEmptyHeroItem()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	auto move = new CAnimImage("IMOBIL", 0, 0, 0, 1);
	auto img  = new CPicture("HPSXXX", move->pos.w + 1);
	auto mana = new CAnimImage("IMANA", 0, 0, move->pos.w + img->pos.w + 2, 1 );

	pos.w = mana->pos.w + mana->pos.x - pos.x;
	pos.h = std::max(std::max<SDLX_Size>(move->pos.h + 1, mana->pos.h + 1), img->pos.h);
}

CHeroList::CHeroItem::CHeroItem(CHeroList *parent, const CGHeroInstance * Hero):
	CListItem(parent),
	hero(Hero)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	movement = new CAnimImage("IMOBIL", 0, 0, 0, 1);
	portrait = new CAnimImage("PortraitsSmall", hero->portrait, 0, movement->pos.w + 1);
	mana     = new CAnimImage("IMANA", 0, 0, movement->pos.w + portrait->pos.w + 2, 1 );

	pos.w = mana->pos.w + mana->pos.x - pos.x;
	pos.h = std::max(std::max<SDLX_Size>(movement->pos.h + 1, mana->pos.h + 1), portrait->pos.h);

	update();
}

void CHeroList::CHeroItem::update()
{
	movement->setFrame(std::min<size_t>(movement->size()-1, hero->movement / 100));
	mana->setFrame(std::min<size_t>(mana->size()-1, hero->mana / 5));
	redraw();
}

CIntObject * CHeroList::CHeroItem::genSelection()
{
	return new CPicture("HPSYYY", movement->pos.w + 1);
}

void CHeroList::CHeroItem::select(bool on)
{
	if (on && adventureInt->selection != hero)
			adventureInt->select(hero);
}

void CHeroList::CHeroItem::open()
{
	LOCPLINT->openHeroWindow(hero);
}

void CHeroList::CHeroItem::showTooltip()
{
	CRClickPopup::createAndPush(hero, GH.current->motion);
}

std::string CHeroList::CHeroItem::getHoverText()
{
	return boost::str(boost::format(CGI->generaltexth->allTexts[15]) % hero->name % hero->type->heroClass->name);
}

CIntObject * CHeroList::createHeroItem(size_t index)
{
	if (LOCPLINT->wanderingHeroes.size() > index)
		return new CHeroItem(this, LOCPLINT->wanderingHeroes[index]);
	return new CEmptyHeroItem();
}

CHeroList::CHeroList(int size, Point position, std::string btnUp, std::string btnDown):
	CList(size, position, btnUp, btnDown, LOCPLINT->wanderingHeroes.size(), 303, 304, std::bind(&CHeroList::createHeroItem, this, _1))
{
}

void CHeroList::select(const CGHeroInstance * hero)
{
	selectIndex(vstd::find_pos(LOCPLINT->wanderingHeroes, hero));
}

void CHeroList::update(const CGHeroInstance * hero)
{
	//this hero is already present, update its status
	for (auto & elem : list->getItems())
	{
		auto item = dynamic_cast<CHeroItem*>(elem);
		if (item && item->hero == hero && vstd::contains(LOCPLINT->wanderingHeroes, hero))
		{
			item->update();
			return;
		}
	}
	//simplest solution for now: reset list and restore selection

	list->resize(LOCPLINT->wanderingHeroes.size());
	if (adventureInt->selection)
	{
		auto hero = dynamic_cast<const CGHeroInstance *>(adventureInt->selection);
		if (hero)
			select(hero);
	}
	CList::update();
}

CIntObject * CTownList::createTownItem(size_t index)
{
	if (LOCPLINT->towns.size() > index)
		return new CTownItem(this, LOCPLINT->towns[index]);
	return new CAnimImage("ITPA", 0);
}

CTownList::CTownItem::CTownItem(CTownList *parent, const CGTownInstance *Town):
	CListItem(parent),
	town(Town)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	picture = new CAnimImage("ITPA", 0);
	pos = picture->pos;
	update();
}

CIntObject * CTownList::CTownItem::genSelection()
{
	return new CAnimImage("ITPA", 1);
}

void CTownList::CTownItem::update()
{
	size_t iconIndex = town->town->clientInfo.icons[town->hasFort()][town->builded >= CGI->modh->settings.MAX_BUILDING_PER_TURN];

	picture->setFrame(iconIndex + 2);
	redraw();
}

void CTownList::CTownItem::select(bool on)
{
	if (on && adventureInt->selection != town)
			adventureInt->select(town);
}

void CTownList::CTownItem::open()
{
	LOCPLINT->openTownWindow(town);
}

void CTownList::CTownItem::showTooltip()
{
	CRClickPopup::createAndPush(town, GH.current->motion);
}

std::string CTownList::CTownItem::getHoverText()
{
	return town->getObjectName();
}

CTownList::CTownList(int size, Point position, std::string btnUp, std::string btnDown):
	CList(size, position, btnUp, btnDown, LOCPLINT->towns.size(),  306, 307, std::bind(&CTownList::createTownItem, this, _1))
{
}

void CTownList::select(const CGTownInstance * town)
{
	selectIndex(vstd::find_pos(LOCPLINT->towns, town));
}

void CTownList::update(const CGTownInstance *)
{
	//simplest solution for now: reset list and restore selection

	list->resize(LOCPLINT->towns.size());
	if (adventureInt->selection)
	{
		auto town = dynamic_cast<const CGTownInstance *>(adventureInt->selection);
		if (town)
			select(town);
	}
	CList::update();
}

const SDL_Color & CMinimapInstance::getTileColor(const int3 & pos)
{
	static const SDL_Color fogOfWar = {0, 0, 0, 255};

	const TerrainTile * tile = LOCPLINT->cb->getTile(pos, false);

	// if tile is not visible it will be black on minimap
	if(!tile)
		return fogOfWar;

	// if object at tile is owned - it will be colored as its owner
	for (const CGObjectInstance *obj : tile->blockingObjects)
	{
		//heroes will be blitted later
		switch (obj->ID)
		{
			case Obj::HERO:
			case Obj::PRISON:
				continue;
		}

		PlayerColor player = obj->getOwner();
		if(player == PlayerColor::NEUTRAL)
			return *graphics->neutralColor;
		else
		if (player < PlayerColor::PLAYER_LIMIT)
			return graphics->playerColors[player.getNum()];
	}

	// else - use terrain color (blocked version or normal)
	if (tile->blocked && (!tile->visitable))
		return parent->colors.find(tile->terType)->second.second;
	else
		return parent->colors.find(tile->terType)->second.first;
}
void CMinimapInstance::tileToPixels (const int3 &tile, int &x, int &y, int toX, int toY)
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();

	double stepX = double(pos.w) / mapSizes.x;
	double stepY = double(pos.h) / mapSizes.y;

	x = toX + stepX * tile.x;
	y = toY + stepY * tile.y;
}

void CMinimapInstance::blitTileWithColor(const SDL_Color &color, const int3 &tile, SDL_Surface *to, int toX, int toY)
{
	//coordinates of rectangle on minimap representing this tile
	// begin - first to blit, end - first NOT to blit
	int xBegin, yBegin, xEnd, yEnd;
	tileToPixels (tile, xBegin, yBegin, toX, toY);
	tileToPixels (int3 (tile.x + 1, tile.y + 1, tile.z), xEnd, yEnd, toX, toY);

	for (int y=yBegin; y<yEnd; y++)
	{
		Uint8 *ptr = (Uint8*)to->pixels + y * to->pitch + xBegin * minimap->format->BytesPerPixel;

		for (int x=xBegin; x<xEnd; x++)
			ColorPutter<4, 1>::PutColor(ptr, color);
	}
}

void CMinimapInstance::refreshTile(const int3 &tile)
{
	blitTileWithColor(getTileColor(int3(tile.x, tile.y, level)), tile, minimap, 0, 0);
}

void CMinimapInstance::drawScaled(int level)
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();

	//size of one map tile on our minimap
	double stepX = double(pos.w) / mapSizes.x;
	double stepY = double(pos.h) / mapSizes.y;

	double currY = 0;
	for (int y=0; y<mapSizes.y; y++, currY += stepY)
	{
		double currX = 0;
		for (int x=0; x<mapSizes.x; x++, currX += stepX)
		{
			const SDL_Color &color = getTileColor(int3(x,y,level));

			//coordinates of rectangle on minimap representing this tile
			// begin - first to blit, end - first NOT to blit
			int xBegin = currX;
			int yBegin = currY;
			int xEnd = currX + stepX;
			int yEnd = currY + stepY;

			for (int y=yBegin; y<yEnd; y++)
			{
				Uint8 *ptr = (Uint8*)minimap->pixels + y * minimap->pitch + xBegin * minimap->format->BytesPerPixel;

				for (int x=xBegin; x<xEnd; x++)
					ColorPutter<4, 1>::PutColor(ptr, color);
			}
		}
	}
}

CMinimapInstance::CMinimapInstance(CMinimap *Parent, int Level):
	parent(Parent),
	minimap(CSDL_Ext::createSurfaceWithBpp<4>(parent->pos.w, parent->pos.h)),
	level(Level)
{
	pos.w = parent->pos.w;
	pos.h = parent->pos.h;
	drawScaled(level);
}

CMinimapInstance::~CMinimapInstance()
{
	SDL_FreeSurface(minimap);
}

void CMinimapInstance::showAll(SDL_Surface *to)
{
	blitAtLoc(minimap, 0, 0, to);

	//draw heroes
	std::vector <const CGHeroInstance *> heroes = LOCPLINT->cb->getHeroesInfo(false); //TODO: do we really need separate function for drawing heroes?
	for(auto & hero : heroes)
	{
		int3 position = hero->getPosition(false);
		if (position.z == level)
		{
			const SDL_Color & color = graphics->playerColors[hero->getOwner().getNum()];
			blitTileWithColor(color, position, to, pos.x, pos.y);
		}
	}
}

std::map<int, std::pair<SDL_Color, SDL_Color> > CMinimap::loadColors(std::string from)
{
	std::map<int, std::pair<SDL_Color, SDL_Color> > ret;

	const JsonNode config(ResourceID(from, EResType::TEXT));

	for(auto &m : config.Struct())
	{
		auto index = boost::find(GameConstants::TERRAIN_NAMES, m.first);
		if (index == std::end(GameConstants::TERRAIN_NAMES))
		{
			logGlobal->error("Error: unknown terrain in terrains.json: %s", m.first);
			continue;
		}
		int terrainID = index - std::begin(GameConstants::TERRAIN_NAMES);

		const JsonVector &unblockedVec = m.second["minimapUnblocked"].Vector();
		SDL_Color normal =
		{
			ui8(unblockedVec[0].Float()),
			ui8(unblockedVec[1].Float()),
			ui8(unblockedVec[2].Float()),
			ui8(255)
		};

		const JsonVector &blockedVec = m.second["minimapBlocked"].Vector();
		SDL_Color blocked =
		{
			ui8(blockedVec[0].Float()),
			ui8(blockedVec[1].Float()),
			ui8(blockedVec[2].Float()),
			ui8(255)
		};

		ret.insert(std::make_pair(terrainID, std::make_pair(normal, blocked)));
	}
	return ret;
}

CMinimap::CMinimap(const Rect &position):
	CIntObject(LCLICK | RCLICK | HOVER | MOVE, position.topLeft()),
	aiShield(nullptr),
	minimap(nullptr),
	level(0),
	colors(loadColors("config/terrains.json"))
{
	pos.w = position.w;
	pos.h = position.h;
}

int3 CMinimap::translateMousePosition()
{
	// 0 = top-left corner, 1 = bottom-right corner
	double dx = double(GH.current->motion.x - pos.x) / pos.w;
	double dy = double(GH.current->motion.y - pos.y) / pos.h;

	int3 mapSizes = LOCPLINT->cb->getMapSize();

	int3 tile (mapSizes.x * dx, mapSizes.y * dy, level);
	return tile;
}

void CMinimap::moveAdvMapSelection()
{
	int3 newLocation = translateMousePosition();
	adventureInt->centerOn(newLocation);

	if (!(adventureInt->active & GENERAL))
		GH.totalRedraw(); //redraw this as well as inactive adventure map
	else
		redraw();//redraw only this
}

void CMinimap::clickLeft(tribool down, bool previousState)
{
	if (down)
		moveAdvMapSelection();
}

void CMinimap::clickRight(tribool down, bool previousState)
{
	adventureInt->handleRightClick(CGI->generaltexth->zelp[291].second, down);
}

void CMinimap::hover(bool on)
{
	if (on)
		GH.statusbar->setText(CGI->generaltexth->zelp[291].first);
	else
		GH.statusbar->clear();
}

void CMinimap::mouseMoved(const SDL_MouseMotionEvent & sEvent)
{
	if(mouseState(EIntObjMouseBtnType::LEFT))
		moveAdvMapSelection();
}

void CMinimap::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	if (minimap)
	{
		int3 mapSizes = LOCPLINT->cb->getMapSize();
		int3 tileCountOnScreen = adventureInt->terrain.tileCountOnScreen();

		//draw radar
		SDL_Rect oldClip;
		SDL_Rect radar =
		{
			si16(adventureInt->position.x * pos.w / mapSizes.x + pos.x),
			si16(adventureInt->position.y * pos.h / mapSizes.y + pos.y),
			ui16(tileCountOnScreen.x * pos.w / mapSizes.x),
			ui16(tileCountOnScreen.y * pos.h / mapSizes.y)
		};

		if (adventureInt->mode == EAdvMapMode::WORLD_VIEW)
		{
			// adjusts radar so that it doesn't go out of map in world view mode (since there's no frame)
			radar.x = std::min<int>(std::max(pos.x, radar.x), pos.x + pos.w - radar.w);
			radar.y = std::min<int>(std::max(pos.y, radar.y), pos.y + pos.h - radar.h);

			if (radar.x < pos.x && radar.y < pos.y)
				return; // whole map is visible at once, no point in redrawing border
		}

		SDL_GetClipRect(to, &oldClip);
		SDL_SetClipRect(to, &pos);
		CSDL_Ext::drawDashedBorder(to, radar, int3(255,75,125));
		SDL_SetClipRect(to, &oldClip);
	}
}

void CMinimap::update()
{
	if (aiShield) //AI turn is going on. There is no need to update minimap
		return;

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	vstd::clear_pointer(minimap);
	minimap = new CMinimapInstance(this, level);
	redraw();
}

void CMinimap::setLevel(int newLevel)
{
	level = newLevel;
	update();
}

void CMinimap::setAIRadar(bool on)
{
	if (on)
	{
		OBJ_CONSTRUCTION_CAPTURING_ALL;
		vstd::clear_pointer(minimap);
		if (!aiShield)
			aiShield = new CPicture("AIShield");
	}
	else
	{
		vstd::clear_pointer(aiShield);
		update();
	}
	// this my happen during AI turn when this interface is inactive
	// force redraw in order to properly update interface
	GH.totalRedraw();
}

void CMinimap::hideTile(const int3 &pos)
{
	if (minimap)
		minimap->refreshTile(pos);
}

void CMinimap::showTile(const int3 &pos)
{
	if (minimap)
		minimap->refreshTile(pos);
}

CInfoBar::CVisibleInfo::CVisibleInfo(Point position):
	CIntObject(0, position),
	aiProgress(nullptr)
{

}

void CInfoBar::CVisibleInfo::show(SDL_Surface *to)
{
	CIntObject::show(to);
	for(auto object : forceRefresh)
		object->showAll(to);
}

void CInfoBar::CVisibleInfo::loadHero(const CGHeroInstance * hero)
{
	assert(children.empty()); // visible info should be re-created to change type

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CPicture("ADSTATHR");
	new CHeroTooltip(Point(0,0), hero);
}

void CInfoBar::CVisibleInfo::loadTown(const CGTownInstance *town)
{
	assert(children.empty()); // visible info should be re-created to change type

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CPicture("ADSTATCS");
	new CTownTooltip(Point(0,0), town);
}

void CInfoBar::CVisibleInfo::playNewDaySound()
{
	if (LOCPLINT->cb->getDate(Date::DAY_OF_WEEK) != 1) // not first day of the week
		CCS->soundh->playSound(soundBase::newDay);
	else
	if (LOCPLINT->cb->getDate(Date::WEEK) != 1) // not first week in month
		CCS->soundh->playSound(soundBase::newWeek);
	else
	if (LOCPLINT->cb->getDate(Date::MONTH) != 1) // not first month
		CCS->soundh->playSound(soundBase::newMonth);
	else
		CCS->soundh->playSound(soundBase::newDay);
}

std::string CInfoBar::CVisibleInfo::getNewDayName()
{
	if (LOCPLINT->cb->getDate(Date::DAY) == 1)
		return "NEWDAY";

	if (LOCPLINT->cb->getDate(Date::DAY) != 1)
		return "NEWDAY";

	switch(LOCPLINT->cb->getDate(Date::WEEK))
	{
	case 1:  return "NEWWEEK1";
	case 2:  return "NEWWEEK2";
	case 3:  return "NEWWEEK3";
	case 4:  return "NEWWEEK4";
	default: assert(0); return "";
	}
}

void CInfoBar::CVisibleInfo::loadDay()
{
	assert(children.empty()); // visible info should be re-created first to change type

	playNewDaySound();

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CShowableAnim(1, 0, getNewDayName(), CShowableAnim::PLAY_ONCE);

	std::string labelText;
	if (LOCPLINT->cb->getDate(Date::DAY_OF_WEEK) == 1 && LOCPLINT->cb->getDate(Date::DAY) != 1) // monday of any week but first - show new week info
		labelText = CGI->generaltexth->allTexts[63] + " " + boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::WEEK));
	else
		labelText = CGI->generaltexth->allTexts[64] + " " + boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::DAY_OF_WEEK));

	forceRefresh.push_back(new CLabel(95, 31, FONT_MEDIUM, CENTER, Colors::WHITE, labelText));
}

void CInfoBar::CVisibleInfo::loadEnemyTurn(PlayerColor player)
{
	assert(children.empty()); // visible info should be re-created to change type

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CPicture("ADSTATNX");
	new CAnimImage("CREST58", player.getNum(), 0, 20, 51);
	new CShowableAnim(99, 51, "HOURSAND");

	// FIXME: currently there is no way to get progress from VCAI
	// if this will change at some point switch this ifdef to enable correct code
#if 0
	//prepare hourglass for updating AI turn
	aiProgress = new CAnimImage("HOURGLAS", 0, 0, 99, 51);
	forceRefresh.push_back(aiProgress);
#else
	//create hourglass that will be always animated ignoring AI status
	new CShowableAnim(99, 51, "HOURGLAS", CShowableAnim::PLAY_ONCE, 40);
#endif
}

void CInfoBar::CVisibleInfo::loadGameStatus()
{
	assert(children.empty()); // visible info should be re-created to change type

	//get amount of halls of each level
	std::vector<int> halls(4, 0);
	for(auto town : LOCPLINT->towns)
	{
		int hallLevel = town->hallLevel();
		//negative value means no village hall, unlikely but possible
		if(hallLevel >= 0)
			halls.at(hallLevel)++;
	}

	std::vector<PlayerColor> allies, enemies;

	//generate list of allies and enemies
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
	{
		if(LOCPLINT->cb->getPlayerStatus(PlayerColor(i), false) == EPlayerStatus::INGAME)
		{
			if (LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, PlayerColor(i)) != PlayerRelations::ENEMIES)
				allies.push_back(PlayerColor(i));
			else
				enemies.push_back(PlayerColor(i));
		}
	}

	//generate component
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CPicture("ADSTATIN");
	auto allyLabel  = new CLabel(10, 106, FONT_SMALL, TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[390] + ":");
	auto enemyLabel = new CLabel(10, 136, FONT_SMALL, TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[391] + ":");

	int posx = allyLabel->pos.w + allyLabel->pos.x - pos.x + 4;
	for(PlayerColor & player : allies)
	{
		auto image = new CAnimImage("ITGFLAGS", player.getNum(), 0, posx, 102);
		posx += image->pos.w;
	}

	posx = enemyLabel->pos.w + enemyLabel->pos.x - pos.x + 4;
	for(PlayerColor & player : enemies)
	{
		auto image = new CAnimImage("ITGFLAGS", player.getNum(), 0, posx, 132);
		posx += image->pos.w;
	}

	for (size_t i=0; i<halls.size(); i++)
	{
		new CAnimImage("itmtl", i, 0, 6 + 42 * i , 11);
		if (halls[i])
			new CLabel( 26 + 42 * i, 64, FONT_SMALL, CENTER, Colors::WHITE, boost::lexical_cast<std::string>(halls[i]));
	}
}

void CInfoBar::CVisibleInfo::loadComponent(const Component & compToDisplay, std::string message)
{
	assert(children.empty()); // visible info should be re-created to change type

	OBJ_CONSTRUCTION_CAPTURING_ALL;

	new CPicture("ADSTATOT", 1);

	auto   comp = new CComponent(compToDisplay);
	comp->moveTo(Point(pos.x+47, pos.y+50));

	new CTextBox(message, Rect(10, 4, 160, 50), 0, FONT_SMALL, CENTER, Colors::WHITE);
}

void CInfoBar::CVisibleInfo::updateEnemyTurn(double progress)
{
	if (aiProgress)
	aiProgress->setFrame((aiProgress->size() - 1) * progress);
}

void CInfoBar::reset(EState newState = EMPTY)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	vstd::clear_pointer(visibleInfo);
	currentObject = nullptr;
	state = newState;
	visibleInfo = new CVisibleInfo(Point(8, 12));
}

void CInfoBar::showSelection()
{
	if (adventureInt->selection)
	{
		auto hero = dynamic_cast<const CGHeroInstance *>(adventureInt->selection);
		if (hero)
		{
			showHeroSelection(hero);
			return;
		}
		auto town = dynamic_cast<const CGTownInstance *>(adventureInt->selection);
		if (town)
		{
			showTownSelection(town);
			return;
		}
	}
	showGameStatus();//FIXME: may be incorrect but shouldn't happen in general
}

void CInfoBar::tick()
{
	removeUsedEvents(TIME);
	if(GH.topInt() == adventureInt)
		showSelection();
}

void CInfoBar::clickLeft(tribool down, bool previousState)
{
	if (down)
	{
		if (state == HERO || state == TOWN)
			showGameStatus();
		else if (state == GAME)
			showDate();
		else
			showSelection();
	}
}

void CInfoBar::clickRight(tribool down, bool previousState)
{
	adventureInt->handleRightClick(CGI->generaltexth->allTexts[109], down);
}

void CInfoBar::hover(bool on)
{
	if (on)
		GH.statusbar->setText(CGI->generaltexth->zelp[292].first);
	else
		GH.statusbar->clear();
}

CInfoBar::CInfoBar(const Rect &position):
	CIntObject(LCLICK | RCLICK | HOVER, position.topLeft()),
	visibleInfo(nullptr),
	state(EMPTY),
	currentObject(nullptr)
{
	pos.w = position.w;
	pos.h = position.h;
	//FIXME: enable some mode? Should be done by advMap::select() when game starts but just in case?
}

void CInfoBar::showDate()
{
	reset(DATE);
	visibleInfo->loadDay();
	setTimer(3000);
	redraw();
}

void CInfoBar::showComponent(const Component & comp, std::string message)
{
	reset(COMPONENT);
	visibleInfo->loadComponent(comp, message);
	setTimer(3000);
	redraw();
}

void CInfoBar::startEnemyTurn(PlayerColor color)
{
	reset(AITURN);
	visibleInfo->loadEnemyTurn(color);
	redraw();
}

void CInfoBar::updateEnemyTurn(double progress)
{
	assert(state == AITURN);
	visibleInfo->updateEnemyTurn(progress);
	redraw();
}

void CInfoBar::showHeroSelection(const CGHeroInstance * hero)
{
	if (!hero)
		return;

	reset(HERO);
	currentObject = hero;
	visibleInfo->loadHero(hero);
	redraw();
}

void CInfoBar::showTownSelection(const CGTownInstance * town)
{
	if (!town)
		return;

	reset(TOWN);
	currentObject = town;
	visibleInfo->loadTown(town);
	redraw();
}

void CInfoBar::showGameStatus()
{
	reset(GAME);
	visibleInfo->loadGameStatus();
	setTimer(3000);
	redraw();
}

void CInGameConsole::show(SDL_Surface * to)
{
	int number = 0;

	std::vector<std::list< std::pair< std::string, int > >::iterator> toDel;

	boost::unique_lock<boost::mutex> lock(texts_mx);
	for(auto it = texts.begin(); it != texts.end(); ++it, ++number)
	{
		Point leftBottomCorner(0, screen->h);
		if(LOCPLINT->battleInt)
		{
			leftBottomCorner = LOCPLINT->battleInt->pos.bottomLeft();
		}
		graphics->fonts[FONT_MEDIUM]->renderTextLeft(to, it->first, Colors::GREEN,
			Point(leftBottomCorner.x + 50, leftBottomCorner.y - texts.size() * 20 - 80 + number*20));

		if(SDL_GetTicks() - it->second > defaultTimeout)
		{
			toDel.push_back(it);
		}
	}

	for(auto & elem : toDel)
	{
		texts.erase(elem);
	}
}

void CInGameConsole::print(const std::string &txt)
{
	boost::unique_lock<boost::mutex> lock(texts_mx);
	int lineLen = conf.go()->ac.outputLineLength;

	if(txt.size() < lineLen)
	{
		texts.push_back(std::make_pair(txt, SDL_GetTicks()));
		if(texts.size() > maxDisplayedTexts)
		{
			texts.pop_front();
		}
	}
	else
	{
		assert(lineLen);
		for(int g=0; g<txt.size() / lineLen + 1; ++g)
		{
			std::string part = txt.substr(g * lineLen, lineLen);
			if(part.size() == 0)
				break;

			texts.push_back(std::make_pair(part, SDL_GetTicks()));
			if(texts.size() > maxDisplayedTexts)
			{
				texts.pop_front();
			}
		}
	}
}

void CInGameConsole::keyPressed (const SDL_KeyboardEvent & key)
{
	if(key.type != SDL_KEYDOWN) return;

	if(!captureAllKeys && key.keysym.sym != SDLK_TAB) return; //because user is not entering any text

	switch(key.keysym.sym)
	{
	case SDLK_TAB:
	case SDLK_ESCAPE:
		{
			if(captureAllKeys)
			{
				captureAllKeys = false;
				endEnteringText(false);
			}
			else if(SDLK_TAB)
			{
				captureAllKeys = true;
				startEnteringText();
			}
			break;
		}
	case SDLK_RETURN: //enter key
		{
			if(enteredText.size() > 0  &&  captureAllKeys)
			{
				captureAllKeys = false;
				endEnteringText(true);
				CCS->soundh->playSound("CHAT");
			}
			break;
		}
	case SDLK_BACKSPACE:
		{
			if(enteredText.size() > 1)
			{
				Unicode::trimRight(enteredText,2);
				enteredText += '_';
				refreshEnteredText();
			}
			break;
		}
	case SDLK_UP: //up arrow
		{
			if(previouslyEntered.size() == 0)
				break;

			if(prevEntDisp == -1)
			{
				prevEntDisp = previouslyEntered.size() - 1;
				enteredText = previouslyEntered[prevEntDisp] + "_";
				refreshEnteredText();
			}
			else if( prevEntDisp > 0)
			{
				--prevEntDisp;
				enteredText = previouslyEntered[prevEntDisp] + "_";
				refreshEnteredText();
			}
			break;
		}
	case SDLK_DOWN: //down arrow
		{
			if(prevEntDisp != -1 && prevEntDisp+1 < previouslyEntered.size())
			{
				++prevEntDisp;
				enteredText = previouslyEntered[prevEntDisp] + "_";
				refreshEnteredText();
			}
			else if(prevEntDisp+1 == previouslyEntered.size()) //useful feature
			{
				prevEntDisp = -1;
				enteredText = "_";
				refreshEnteredText();
			}
			break;
		}
	default:
		{
			break;
		}
	}
}

void CInGameConsole::textInputed(const SDL_TextInputEvent & event)
{
	if(!captureAllKeys || enteredText.size() == 0)
		return;
	enteredText.resize(enteredText.size()-1);

	enteredText += event.text;
	enteredText += "_";

	refreshEnteredText();
}

void CInGameConsole::textEdited(const SDL_TextEditingEvent & event)
{
 //do nothing here
}

void CInGameConsole::startEnteringText()
{
	CSDL_Ext::startTextInput(&pos);

	enteredText = "_";
	if(GH.topInt() == adventureInt)
	{
		GH.statusbar->alignment = TOPLEFT;
		GH.statusbar->setText(enteredText);

		//Prevent changes to the text from mouse interaction with the adventure map
		GH.statusbar->lock(true);
	}
	else if(LOCPLINT->battleInt)
	{
		LOCPLINT->battleInt->console->ingcAlter = enteredText;
	}
}

void CInGameConsole::endEnteringText(bool printEnteredText)
{
	CSDL_Ext::stopTextInput();

	prevEntDisp = -1;
	if(printEnteredText)
	{
		std::string txt = enteredText.substr(0, enteredText.size()-1);
		LOCPLINT->cb->sendMessage(txt, LOCPLINT->getSelection());
		previouslyEntered.push_back(txt);
		//print(txt);
	}
	enteredText = "";
	if(GH.topInt() == adventureInt)
	{
		GH.statusbar->alignment = CENTER;
		GH.statusbar->lock(false);
		GH.statusbar->clear();
	}
	else if(LOCPLINT->battleInt)
	{
		LOCPLINT->battleInt->console->ingcAlter = "";
	}
}

void CInGameConsole::refreshEnteredText()
{
	if(GH.topInt() == adventureInt)
	{
		GH.statusbar->lock(false);
		GH.statusbar->clear();
		GH.statusbar->setText(enteredText);
		GH.statusbar->lock(true);
	}
	else if(LOCPLINT->battleInt)
	{
		LOCPLINT->battleInt->console->ingcAlter = enteredText;
	}
}

CInGameConsole::CInGameConsole() : prevEntDisp(-1), defaultTimeout(10000), maxDisplayedTexts(10)
{
	addUsedEvents(KEYBOARD | TEXTINPUT);
}

CAdvMapPanel::CAdvMapPanel(SDL_Surface * bg, Point position)
	: CIntObject(),
	  background(bg)
{
	defActions = 255;
	recActions = 255;
	pos.x += position.x;
	pos.y += position.y;
	if (bg)
	{
		pos.w = bg->w;
		pos.h = bg->h;
	}
}

CAdvMapPanel::~CAdvMapPanel()
{
	if (background)
		SDL_FreeSurface(background);
}

void CAdvMapPanel::addChildColorableButton(CButton * btn)
{
	buttons.push_back(btn);
	addChildToPanel(btn, ACTIVATE | DEACTIVATE);
}

void CAdvMapPanel::setPlayerColor(const PlayerColor & clr)
{
	for (auto &btn : buttons)
	{
		btn->setPlayerColor(clr);
	}
}

void CAdvMapPanel::showAll(SDL_Surface * to)
{
	if (background)
		blitAt(background, pos.x, pos.y, to);

	CIntObject::showAll(to);
}

void CAdvMapPanel::addChildToPanel(CIntObject * obj, ui8 actions)
{
	obj->recActions |= actions | SHOWALL;
	addChild(obj, false);
}

CAdvMapWorldViewPanel::CAdvMapWorldViewPanel(std::shared_ptr<CAnimation> _icons, SDL_Surface * bg, Point position, int spaceBottom, const PlayerColor &color)
	: CAdvMapPanel(bg, position), icons(_icons)
{
	fillerHeight = bg ? spaceBottom - pos.y - pos.h : 0;

	if (fillerHeight > 0)
	{
		tmpBackgroundFiller = CMessage::drawDialogBox(pos.w, fillerHeight, color);
	}
	else
		tmpBackgroundFiller = nullptr;
}

CAdvMapWorldViewPanel::~CAdvMapWorldViewPanel()
{
	if (tmpBackgroundFiller)
		SDL_FreeSurface(tmpBackgroundFiller);
}

void CAdvMapWorldViewPanel::recolorIcons(const PlayerColor &color, int indexOffset)
{
	assert(iconsData.size() == currentIcons.size());

	for(size_t idx = 0; idx < iconsData.size(); idx++)
	{
		const auto & data = iconsData.at(idx);
		currentIcons[idx]->setFrame(data.first + indexOffset);
	}

	if (fillerHeight > 0)
	{
		if (tmpBackgroundFiller)
			SDL_FreeSurface(tmpBackgroundFiller);
		tmpBackgroundFiller = CMessage::drawDialogBox(pos.w, fillerHeight, color);
	}
}

void CAdvMapWorldViewPanel::addChildIcon(std::pair<int, Point> data, int indexOffset)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	iconsData.push_back(data);
	currentIcons.push_back(new CAnimImage(icons, data.first + indexOffset, 0, data.second.x, data.second.y));
}

void CAdvMapWorldViewPanel::showAll(SDL_Surface * to)
{
	if (tmpBackgroundFiller)
	{
		blitAt(tmpBackgroundFiller, pos.x, pos.y + pos.h, to);
	}

	CAdvMapPanel::showAll(to);
}
