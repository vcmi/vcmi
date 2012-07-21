#include "StdInc.h"
#include "AdventureMapClasses.h"

#include "../CCallback.h"
#include "../lib/JsonNode.h"
#include "../lib/map.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CGameState.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/NetPacks.h"
#include "../lib/CHeroHandler.h"
#include "CAdvmapInterface.h"
#include "CAnimation.h"
#include "CGameInfo.h"
#include "CPlayerInterface.h"
#include "CMusicHandler.h"
#include "Graphics.h"
#include "GUIClasses.h"
#include "UIFramework/CGuiHandler.h"
#include "UIFramework/SDL_Pixels.h"

/*
 * CAdventureMapClasses.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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
		GH.statusbar->print(getHoverText());
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
	scrollUp = new CAdventureMapButton(CGI->generaltexth->zelp[helpUp], 0, 0, 0, btnUp);
	list = new CListBox(create, destroy, Point(1,scrollUp->pos.h), Point(0, 32), size, listAmount);

	//assign callback only after list was created
	scrollUp->callback = boost::bind(&CListBox::moveToPrev, list);
	scrollDown = new CAdventureMapButton(CGI->generaltexth->zelp[helpDown], boost::bind(&CListBox::moveToNext, list), 0, scrollUp->pos.h + 32*size, btnDown);

	scrollDown->callback += boost::bind(&CList::update, this);
	scrollUp->callback += boost::bind(&CList::update, this);

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
	int index = getSelectedIndex();
	if (index < 0)
		selectIndex(0);
	else if (index + 1 < list->size())
		selectIndex(index+1);
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
	pos.h = std::max(std::max<ui16>(move->pos.h + 1, mana->pos.h + 1), img->pos.h);
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
	pos.h = std::max(std::max<ui16>(movement->pos.h + 1, mana->pos.h + 1), portrait->pos.h);

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
    CList(size, position, btnUp, btnDown, LOCPLINT->wanderingHeroes.size(), 303, 304, boost::bind(&CHeroList::createHeroItem, this, _1))
{
}

void CHeroList::select(const CGHeroInstance * hero)
{
	selectIndex(vstd::find_pos(LOCPLINT->wanderingHeroes, hero));
}

void CHeroList::update(const CGHeroInstance * hero)
{
	//this hero is already present, update its status
	for (auto iter = list->getItems().begin(); iter != list->getItems().end(); iter++)
	{
		auto item = dynamic_cast<CHeroItem*>(*iter);
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
	size_t iconIndex = town->subID*2;
	if (!town->hasFort())
		iconIndex += GameConstants::F_NUMBER*2;

	if(town->builded >= GameConstants::MAX_BUILDING_PER_TURN)
		iconIndex++;

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
	return town->hoverName;
}

CTownList::CTownList(int size, Point position, std::string btnUp, std::string btnDown):
    CList(size, position, btnUp, btnDown, LOCPLINT->towns.size(),  306, 307, boost::bind(&CTownList::createTownItem, this, _1))
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
	BOOST_FOREACH(const CGObjectInstance *obj, tile->blockingObjects)
	{
		//heroes will be blitted later
		if (obj->ID == GameConstants::HEROI_TYPE)
			continue;

		int player = obj->getOwner();
		if(player == GameConstants::NEUTRAL_PLAYER)
			return *graphics->neutralColor;
		else
		if (player < GameConstants::PLAYER_LIMIT)
			return graphics->playerColors[player];
	}

	// else - use terrain color (blocked version or normal)
	if (tile->blocked && (!tile->visitable))
		return parent->colors.find(tile->tertype)->second.second;
	else
		return parent->colors.find(tile->tertype)->second.first;
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
	std::vector <const CGHeroInstance *> heroes = LOCPLINT->cb->getHeroesInfo(false);
	BOOST_FOREACH(auto & hero, heroes)
	{
		int3 position = hero->getPosition(false);
		if (position.z == level)
		{
			const SDL_Color & color = graphics->playerColors[hero->getOwner()];
			blitTileWithColor(color, position, to, pos.x, pos.y);
		}
	}
}

std::map<int, std::pair<SDL_Color, SDL_Color> > CMinimap::loadColors(std::string from)
{
	std::map<int, std::pair<SDL_Color, SDL_Color> > ret;

	const JsonNode config(GameConstants::DATA_DIR + from);

	BOOST_FOREACH(const JsonNode &m, config["MinimapColors"].Vector())
	{
		int id = m["terrain_id"].Float();

		const JsonVector &unblockedVec = m["unblocked"].Vector();
		SDL_Color normal =
		{
			ui8(unblockedVec[0].Float()),
			ui8(unblockedVec[1].Float()),
			ui8(unblockedVec[2].Float()),
			ui8(255)
		};

		const JsonVector &blockedVec = m["blocked"].Vector();
		SDL_Color blocked =
		{
			ui8(blockedVec[0].Float()),
			ui8(blockedVec[1].Float()),
			ui8(blockedVec[2].Float()),
			ui8(255)
		};

		ret.insert(std::make_pair(id, std::make_pair(normal, blocked)));
	}
	return ret;
}

CMinimap::CMinimap(const Rect &position):
    CIntObject(LCLICK | RCLICK | HOVER | MOVE, position.topLeft()),
    aiShield(nullptr),
    minimap(nullptr),
    level(0),
    colors(loadColors("/config/minimap.json"))
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

	GH.totalRedraw(); //redraw this as well as adventure map (which may be inactive)
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
		GH.statusbar->print(CGI->generaltexth->zelp[291].first);
	else
		GH.statusbar->clear();
}

void CMinimap::mouseMoved(const SDL_MouseMotionEvent & sEvent)
{
	if (pressedL)
		moveAdvMapSelection();
}

void CMinimap::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	if (minimap)
	{
		int3 mapSizes = LOCPLINT->cb->getMapSize();

		//draw radar
		SDL_Rect oldClip;
		SDL_Rect radar =
		{
			si16(adventureInt->position.x * pos.w / mapSizes.x + pos.x),
			si16(adventureInt->position.y * pos.h / mapSizes.y + pos.y),
			ui16(adventureInt->terrain.tilesw * pos.w / mapSizes.x),
			ui16(adventureInt->terrain.tilesh * pos.h / mapSizes.y)
		};

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
	BOOST_FOREACH(auto object, forceRefresh)
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
	if (LOCPLINT->cb->getDate(1) != 1) // not first day of the week
		CCS->soundh->playSound(soundBase::newDay);
	else
	if (LOCPLINT->cb->getDate(2) != 1) // not first week in month
		CCS->soundh->playSound(soundBase::newWeek);
	else
	if (LOCPLINT->cb->getDate(3) != 1) // not first month
		CCS->soundh->playSound(soundBase::newMonth);
	else
		CCS->soundh->playSound(soundBase::newDay);
}

std::string CInfoBar::CVisibleInfo::getNewDayName()
{
	if (LOCPLINT->cb->getDate(0) == 1)
		return "NEWDAY";

	if (LOCPLINT->cb->getDate(1) != 1)
		return "NEWDAY";

	switch(LOCPLINT->cb->getDate(2))
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
	if (LOCPLINT->cb->getDate(1) == 1 && LOCPLINT->cb->getDate(0) != 1) // monday of any week but first - show new week info
		labelText = CGI->generaltexth->allTexts[63] + " " + boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(2));
	else
		labelText = CGI->generaltexth->allTexts[64] + " " + boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(1));

	forceRefresh.push_back(new CLabel(95, 31, FONT_MEDIUM, CENTER, Colors::Cornsilk, labelText));
}

void CInfoBar::CVisibleInfo::loadEnemyTurn(int player)
{
	assert(children.empty()); // visible info should be re-created to change type

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CPicture("ADSTATNX");
	new CAnimImage("CREST58", player, 0, 20, 51);
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
	BOOST_FOREACH(auto town, LOCPLINT->towns)
		halls[town->hallLevel()]++;

	std::vector<int> allies, enemies;

	//generate list of allies and enemies
	for(int i = 0; i < GameConstants::PLAYER_LIMIT; i++)
	{
		if(LOCPLINT->cb->getPlayerStatus(i) == PlayerState::INGAME)
		{
			if (LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, i) > 0)
				allies.push_back(i);
			else
				enemies.push_back(i);
		}
	}

	//generate component
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CPicture("ADSTATIN");
	auto allyLabel  = new CLabel(10, 106, FONT_SMALL, TOPLEFT, Colors::Cornsilk, CGI->generaltexth->allTexts[390] + ":");
	auto enemyLabel = new CLabel(10, 136, FONT_SMALL, TOPLEFT, Colors::Cornsilk, CGI->generaltexth->allTexts[391] + ":");

	int posx = allyLabel->pos.w + allyLabel->pos.x - pos.x + 4;
	BOOST_FOREACH(int & player, allies)
	{
		auto image = new CAnimImage("ITGFLAGS", player, 0, posx, 102);
		posx += image->pos.w;
	}

	posx = enemyLabel->pos.w + enemyLabel->pos.x - pos.x + 4;
	BOOST_FOREACH(int & player, enemies)
	{
		auto image = new CAnimImage("ITGFLAGS", player, 0, posx, 132);
		posx += image->pos.w;
	}

	for (size_t i=0; i<halls.size(); i++)
	{
		new CAnimImage("itmtl", i, 0, 6 + 42 * i , 11);
		if (halls[i])
			new CLabel( 26 + 42 * i, 64, FONT_SMALL, CENTER, Colors::Cornsilk, boost::lexical_cast<std::string>(halls[i]));
	}
}

void CInfoBar::CVisibleInfo::loadComponent(const Component compToDisplay, std::string message)
{
	assert(children.empty()); // visible info should be re-created to change type

	OBJ_CONSTRUCTION_CAPTURING_ALL;

	new CPicture("ADSTATOT");

	auto comp = new CComponent(compToDisplay);
	comp->moveTo(Point(pos.x+52, pos.y+54));

	new CTextBox(message, Rect(8, 8, 164, 50), 0, FONT_SMALL, CENTER, Colors::Cornsilk);
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
		GH.statusbar->print(CGI->generaltexth->zelp[292].first);
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

void CInfoBar::showComponent(const Component comp, std::string message)
{
	reset(COMPONENT);
	visibleInfo->loadComponent(comp, message);
	setTimer(3000);
	redraw();
}

void CInfoBar::startEnemyTurn(ui8 color)
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
