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

#include <SDL_timer.h>

#include "MiscWidgets.h"
#include "CComponent.h"
#include "Images.h"

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../mainmenu/CMainMenu.h"

#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Pixels.h"
#include "../gui/CAnimation.h"

#include "../windows/InfoWindows.h"
#include "../windows/CAdvmapInterface.h"

#include "../battle/BattleInterfaceClasses.h"
#include "../battle/BattleInterface.h"

#include "../../CCallback.h"
#include "../../lib/StartInfo.h"
#include "../../lib/CGameState.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CModHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"
#include "ClientCommandManager.h"

#include <SDL_surface.h>
#include <SDL_keyboard.h>
#include <SDL_events.h>

CList::CListItem::CListItem(CList * Parent)
	: CIntObject(LCLICK | RCLICK | HOVER),
	parent(Parent),
	selection()
{
	defActions = 255-DISPOSE;
}

CList::CListItem::~CListItem()
{
}

void CList::CListItem::clickRight(tribool down, bool previousState)
{
	if (down == true)
		showTooltip();
}

void CList::CListItem::clickLeft(tribool down, bool previousState)
{
	if(down == true)
	{
		//second click on already selected item
		if(parent->selected == this->shared_from_this())
		{
			open();
		}
		else
		{
			//first click - switch selection
			parent->select(this->shared_from_this());
		}
	}
}

void CList::CListItem::hover(bool on)
{
	if (on)
		GH.statusbar->write(getHoverText());
	else
		GH.statusbar->clear();
}

void CList::CListItem::onSelect(bool on)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	selection.reset();
	if(on)
		selection = genSelection();
	select(on);
	GH.totalRedraw();
}

CList::CList(int Size, Point position, std::string btnUp, std::string btnDown, size_t listAmount, int helpUp, int helpDown, CListBox::CreateFunc create)
	: CIntObject(0, position),
	size(Size),
	selected(nullptr)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	scrollUp = std::make_shared<CButton>(Point(0, 0), btnUp, CGI->generaltexth->zelp[helpUp]);
	scrollDown = std::make_shared<CButton>(Point(0, scrollUp->pos.h + 32*(int)size), btnDown, CGI->generaltexth->zelp[helpDown]);

	listBox = std::make_shared<CListBox>(create, Point(1,scrollUp->pos.h), Point(0, 32), size, listAmount);

	//assign callback only after list was created
	scrollUp->addCallback(std::bind(&CListBox::moveToPrev, listBox));
	scrollDown->addCallback(std::bind(&CListBox::moveToNext, listBox));

	scrollUp->addCallback(std::bind(&CList::update, this));
	scrollDown->addCallback(std::bind(&CList::update, this));

	update();
}

void CList::update()
{
	bool onTop = listBox->getPos() == 0;
	bool onBottom = listBox->getPos() + size >= listBox->size();

	scrollUp->block(onTop);
	scrollDown->block(onBottom);
}

void CList::select(std::shared_ptr<CListItem> which)
{
	if(selected == which)
		return;

	if(selected)
		selected->onSelect(false);

	selected = which;
	if(which)
	{
		which->onSelect(true);
		onSelect();
	}
}

int CList::getSelectedIndex()
{
	return static_cast<int>(listBox->getIndexOf(selected));
}

void CList::selectIndex(int which)
{
	if(which < 0)
	{
		if(selected)
			select(nullptr);
	}
	else
	{
		listBox->scrollTo(which);
		update();
		select(std::dynamic_pointer_cast<CListItem>(listBox->getItem(which)));
	}
}

void CList::selectNext()
{
	int index = getSelectedIndex() + 1;
	if(index >= listBox->size())
		index = 0;
	selectIndex(index);
}

void CList::selectPrev()
{
	int index = getSelectedIndex();
	if(index <= 0)
		selectIndex(0);
	else
		selectIndex(index-1);
}

CHeroList::CEmptyHeroItem::CEmptyHeroItem()
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	movement = std::make_shared<CAnimImage>("IMOBIL", 0, 0, 0, 1);
	portrait = std::make_shared<CPicture>("HPSXXX", movement->pos.w + 1, 0);
	mana = std::make_shared<CAnimImage>("IMANA", 0, 0, movement->pos.w + portrait->pos.w + 2, 1 );

	pos.w = mana->pos.w + mana->pos.x - pos.x;
	pos.h = std::max(std::max<int>(movement->pos.h + 1, mana->pos.h + 1), portrait->pos.h);
}

CHeroList::CHeroItem::CHeroItem(CHeroList *parent, const CGHeroInstance * Hero)
	: CListItem(parent),
	hero(Hero)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	movement = std::make_shared<CAnimImage>("IMOBIL", 0, 0, 0, 1);
	portrait = std::make_shared<CAnimImage>("PortraitsSmall", hero->portrait, 0, movement->pos.w + 1);
	mana = std::make_shared<CAnimImage>("IMANA", 0, 0, movement->pos.w + portrait->pos.w + 2, 1);

	pos.w = mana->pos.w + mana->pos.x - pos.x;
	pos.h = std::max(std::max<int>(movement->pos.h + 1, mana->pos.h + 1), portrait->pos.h);

	update();
}

void CHeroList::CHeroItem::update()
{
	movement->setFrame(std::min<size_t>(movement->size()-1, hero->movement / 100));
	mana->setFrame(std::min<size_t>(mana->size()-1, hero->mana / 5));
	redraw();
}

std::shared_ptr<CIntObject> CHeroList::CHeroItem::genSelection()
{
	return std::make_shared<CPicture>("HPSYYY", movement->pos.w + 1, 0);
}

void CHeroList::CHeroItem::select(bool on)
{
	if(on && adventureInt->selection != hero)
		adventureInt->select(hero);
}

void CHeroList::CHeroItem::open()
{
	LOCPLINT->openHeroWindow(hero);
}

void CHeroList::CHeroItem::showTooltip()
{
	CRClickPopup::createAndPush(hero, GH.getCursorPosition());
}

std::string CHeroList::CHeroItem::getHoverText()
{
	return boost::str(boost::format(CGI->generaltexth->allTexts[15]) % hero->getNameTranslated() % hero->type->heroClass->getNameTranslated());
}

std::shared_ptr<CIntObject> CHeroList::createHeroItem(size_t index)
{
	if (LOCPLINT->wanderingHeroes.size() > index)
		return std::make_shared<CHeroItem>(this, LOCPLINT->wanderingHeroes[index]);
	return std::make_shared<CEmptyHeroItem>();
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
	for(auto & elem : listBox->getItems())
	{
		auto item = std::dynamic_pointer_cast<CHeroItem>(elem);
		if(item && item->hero == hero && vstd::contains(LOCPLINT->wanderingHeroes, hero))
		{
			item->update();
			return;
		}
	}
	//simplest solution for now: reset list and restore selection

	listBox->resize(LOCPLINT->wanderingHeroes.size());
	if (adventureInt->selection)
	{
		auto selectedHero = dynamic_cast<const CGHeroInstance *>(adventureInt->selection);
		if (selectedHero)
			select(selectedHero);
	}
	CList::update();
}

std::shared_ptr<CIntObject> CTownList::createTownItem(size_t index)
{
	if (LOCPLINT->towns.size() > index)
		return std::make_shared<CTownItem>(this, LOCPLINT->towns[index]);
	return std::make_shared<CAnimImage>("ITPA", 0);
}

CTownList::CTownItem::CTownItem(CTownList *parent, const CGTownInstance *Town):
	CListItem(parent),
	town(Town)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	picture = std::make_shared<CAnimImage>("ITPA", 0);
	pos = picture->pos;
	update();
}

std::shared_ptr<CIntObject> CTownList::CTownItem::genSelection()
{
	return std::make_shared<CAnimImage>("ITPA", 1);
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
	CRClickPopup::createAndPush(town, GH.getCursorPosition());
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

	listBox->resize(LOCPLINT->towns.size());
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
	const TerrainTile * tile = LOCPLINT->cb->getTile(pos, false);

	// if tile is not visible it will be black on minimap
	if(!tile)
		return Colors::BLACK;

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
	const auto & colorPair = parent->colors.find(tile->terType->getId())->second;
	if (tile->blocked && (!tile->visitable))
		return colorPair.second;
	else
		return colorPair.first;
}
void CMinimapInstance::tileToPixels (const int3 &tile, int &x, int &y, int toX, int toY)
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();

	double stepX = double(pos.w) / mapSizes.x;
	double stepY = double(pos.h) / mapSizes.y;

	x = static_cast<int>(toX + stepX * tile.x);
	y = static_cast<int>(toY + stepY * tile.y);
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
		uint8_t *ptr = (uint8_t*)to->pixels + y * to->pitch + xBegin * minimap->format->BytesPerPixel;

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
			int xBegin = static_cast<int>(currX);
			int yBegin = static_cast<int>(currY);
			int xEnd = static_cast<int>(currX + stepX);
			int yEnd = static_cast<int>(currY + stepY);

			for (int y=yBegin; y<yEnd; y++)
			{
				uint8_t *ptr = (uint8_t*)minimap->pixels + y * minimap->pitch + xBegin * minimap->format->BytesPerPixel;

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

void CMinimapInstance::showAll(SDL_Surface * to)
{
	blitAtLoc(minimap, 0, 0, to);

	//draw heroes
	std::vector <const CGHeroInstance *> heroes = LOCPLINT->cb->getHeroesInfo(false); //TODO: do we really need separate function for drawing heroes?
	for(auto & hero : heroes)
	{
		int3 position = hero->visitablePos();
		if(position.z == level)
		{
			const SDL_Color & color = graphics->playerColors[hero->getOwner().getNum()];
			blitTileWithColor(color, position, to, pos.x, pos.y);
		}
	}
}

std::map<TerrainId, std::pair<SDL_Color, SDL_Color> > CMinimap::loadColors()
{
	std::map<TerrainId, std::pair<SDL_Color, SDL_Color> > ret;

	for(const auto & terrain : CGI->terrainTypeHandler->objects)
	{
		SDL_Color normal = CSDL_Ext::toSDL(terrain->minimapUnblocked);
		SDL_Color blocked = CSDL_Ext::toSDL(terrain->minimapBlocked);

		ret[terrain->getId()] = std::make_pair(normal, blocked);
	}
	return ret;
}

CMinimap::CMinimap(const Rect & position)
	: CIntObject(LCLICK | RCLICK | HOVER | MOVE, position.topLeft()),
	level(0),
	colors(loadColors())
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	pos.w = position.w;
	pos.h = position.h;

	aiShield = std::make_shared<CPicture>("AIShield");
	aiShield->disable();
}

int3 CMinimap::translateMousePosition()
{
	// 0 = top-left corner, 1 = bottom-right corner
	double dx = double(GH.getCursorPosition().x - pos.x) / pos.w;
	double dy = double(GH.getCursorPosition().y - pos.y) / pos.h;

	int3 mapSizes = LOCPLINT->cb->getMapSize();

	int3 tile ((si32)(mapSizes.x * dx), (si32)(mapSizes.y * dy), level);
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
	if(down)
		moveAdvMapSelection();
}

void CMinimap::clickRight(tribool down, bool previousState)
{
	adventureInt->handleRightClick(CGI->generaltexth->zelp[291].second, down);
}

void CMinimap::hover(bool on)
{
	if(on)
		GH.statusbar->write(CGI->generaltexth->zelp[291].first);
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
	if(minimap)
	{
		int3 mapSizes = LOCPLINT->cb->getMapSize();
		int3 tileCountOnScreen = adventureInt->terrain.tileCountOnScreen();

		//draw radar
		Rect oldClip;
		Rect radar =
		{
			si16(adventureInt->position.x * pos.w / mapSizes.x + pos.x),
			si16(adventureInt->position.y * pos.h / mapSizes.y + pos.y),
			ui16(tileCountOnScreen.x * pos.w / mapSizes.x),
			ui16(tileCountOnScreen.y * pos.h / mapSizes.y)
		};

		if(adventureInt->mode == EAdvMapMode::WORLD_VIEW)
		{
			// adjusts radar so that it doesn't go out of map in world view mode (since there's no frame)
			radar.x = std::min<int>(std::max(pos.x, radar.x), pos.x + pos.w - radar.w);
			radar.y = std::min<int>(std::max(pos.y, radar.y), pos.y + pos.h - radar.h);

			if(radar.x < pos.x && radar.y < pos.y)
				return; // whole map is visible at once, no point in redrawing border
		}

		CSDL_Ext::getClipRect(to, oldClip);
		CSDL_Ext::setClipRect(to, pos);
		CSDL_Ext::drawDashedBorder(to, radar, Colors::PURPLE);
		CSDL_Ext::setClipRect(to, oldClip);
	}
}

void CMinimap::update()
{
	if(aiShield->recActions & UPDATE) //AI turn is going on. There is no need to update minimap
		return;

	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	minimap = std::make_shared<CMinimapInstance>(this, level);
	redraw();
}

void CMinimap::setLevel(int newLevel)
{
	level = newLevel;
	update();
}

void CMinimap::setAIRadar(bool on)
{
	if(on)
	{
		aiShield->enable();
		minimap.reset();
	}
	else
	{
		aiShield->disable();
		update();
	}
	// this my happen during AI turn when this interface is inactive
	// force redraw in order to properly update interface
	GH.totalRedraw();
}

void CMinimap::hideTile(const int3 &pos)
{
	if(minimap)
		minimap->refreshTile(pos);
}

void CMinimap::showTile(const int3 &pos)
{
	if(minimap)
		minimap->refreshTile(pos);
}

CInfoBar::CVisibleInfo::CVisibleInfo()
	: CIntObject(0, Point(8, 12))
{
}

void CInfoBar::CVisibleInfo::show(SDL_Surface * to)
{
	CIntObject::show(to);
	for(auto object : forceRefresh)
		object->showAll(to);
}

CInfoBar::EmptyVisibleInfo::EmptyVisibleInfo()
{
}

CInfoBar::VisibleHeroInfo::VisibleHeroInfo(const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	background = std::make_shared<CPicture>("ADSTATHR");
	heroTooltip = std::make_shared<CHeroTooltip>(Point(0,0), hero);
}

CInfoBar::VisibleTownInfo::VisibleTownInfo(const CGTownInstance * town)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	background = std::make_shared<CPicture>("ADSTATCS");
	townTooltip = std::make_shared<CTownTooltip>(Point(0,0), town);
}

CInfoBar::VisibleDateInfo::VisibleDateInfo()
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	animation = std::make_shared<CShowableAnim>(1, 0, getNewDayName(), CShowableAnim::PLAY_ONCE);

	std::string labelText;
	if(LOCPLINT->cb->getDate(Date::DAY_OF_WEEK) == 1 && LOCPLINT->cb->getDate(Date::DAY) != 1) // monday of any week but first - show new week info
		labelText = CGI->generaltexth->allTexts[63] + " " + boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::WEEK));
	else
		labelText = CGI->generaltexth->allTexts[64] + " " + boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::DAY_OF_WEEK));

	label = std::make_shared<CLabel>(95, 31, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, labelText);

	forceRefresh.push_back(label);
}

std::string CInfoBar::VisibleDateInfo::getNewDayName()
{
	if(LOCPLINT->cb->getDate(Date::DAY) == 1)
		return "NEWDAY";

	if(LOCPLINT->cb->getDate(Date::DAY) != 1)
		return "NEWDAY";

	switch(LOCPLINT->cb->getDate(Date::WEEK))
	{
	case 1:
		return "NEWWEEK1";
	case 2:
		return "NEWWEEK2";
	case 3:
		return "NEWWEEK3";
	case 4:
		return "NEWWEEK4";
	default:
		return "";
	}
}

CInfoBar::VisibleEnemyTurnInfo::VisibleEnemyTurnInfo(PlayerColor player)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	background = std::make_shared<CPicture>("ADSTATNX");
	banner = std::make_shared<CAnimImage>("CREST58", player.getNum(), 0, 20, 51);
	sand = std::make_shared<CShowableAnim>(99, 51, "HOURSAND");
	glass = std::make_shared<CShowableAnim>(99, 51, "HOURGLAS", CShowableAnim::PLAY_ONCE, 40);
}

CInfoBar::VisibleGameStatusInfo::VisibleGameStatusInfo()
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
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
			if(LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, PlayerColor(i)) != PlayerRelations::ENEMIES)
				allies.push_back(PlayerColor(i));
			else
				enemies.push_back(PlayerColor(i));
		}
	}

	//generate widgets
	background = std::make_shared<CPicture>("ADSTATIN");
	allyLabel = std::make_shared<CLabel>(10, 106, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[390] + ":");
	enemyLabel = std::make_shared<CLabel>(10, 136, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[391] + ":");

	int posx = allyLabel->pos.w + allyLabel->pos.x - pos.x + 4;
	for(PlayerColor & player : allies)
	{
		auto image = std::make_shared<CAnimImage>("ITGFLAGS", player.getNum(), 0, posx, 102);
		posx += image->pos.w;
		flags.push_back(image);
	}

	posx = enemyLabel->pos.w + enemyLabel->pos.x - pos.x + 4;
	for(PlayerColor & player : enemies)
	{
		auto image = std::make_shared<CAnimImage>("ITGFLAGS", player.getNum(), 0, posx, 132);
		posx += image->pos.w;
		flags.push_back(image);
	}

	for(size_t i=0; i<halls.size(); i++)
	{
		hallIcons.push_back(std::make_shared<CAnimImage>("itmtl", i, 0, 6 + 42 * (int)i , 11));
		if(halls[i])
			hallLabels.push_back(std::make_shared<CLabel>( 26 + 42 * (int)i, 64, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, boost::lexical_cast<std::string>(halls[i])));
	}
}

CInfoBar::VisibleComponentInfo::VisibleComponentInfo(const Component & compToDisplay, std::string message)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	background = std::make_shared<CPicture>("ADSTATOT", 1, 0);

	comp = std::make_shared<CComponent>(compToDisplay);
	comp->moveTo(Point(pos.x+47, pos.y+50));

	text = std::make_shared<CTextBox>(message, Rect(10, 4, 160, 50), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
}

void CInfoBar::playNewDaySound()
{
	if(LOCPLINT->cb->getDate(Date::DAY_OF_WEEK) != 1) // not first day of the week
		CCS->soundh->playSound(soundBase::newDay);
	else if(LOCPLINT->cb->getDate(Date::WEEK) != 1) // not first week in month
		CCS->soundh->playSound(soundBase::newWeek);
	else if(LOCPLINT->cb->getDate(Date::MONTH) != 1) // not first month
		CCS->soundh->playSound(soundBase::newMonth);
	else
		CCS->soundh->playSound(soundBase::newDay);
}

void CInfoBar::reset()
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	state = EMPTY;
	visibleInfo = std::make_shared<EmptyVisibleInfo>();
}

void CInfoBar::showSelection()
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	if(adventureInt->selection)
	{
		if(auto hero = dynamic_cast<const CGHeroInstance *>(adventureInt->selection))
		{
			showHeroSelection(hero);
			return;
		}
		else if(auto town = dynamic_cast<const CGTownInstance *>(adventureInt->selection))
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
	if(down)
	{
		if(state == HERO || state == TOWN)
			showGameStatus();
		else if(state == GAME)
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
	if(on)
		GH.statusbar->write(CGI->generaltexth->zelp[292].first);
	else
		GH.statusbar->clear();
}

CInfoBar::CInfoBar(const Rect & position)
	: CIntObject(LCLICK | RCLICK | HOVER, position.topLeft()),
	state(EMPTY)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	pos.w = position.w;
	pos.h = position.h;
	reset();
}

void CInfoBar::showDate()
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	playNewDaySound();
	state = DATE;
	visibleInfo = std::make_shared<VisibleDateInfo>();
	setTimer(3000);
	redraw();
}

void CInfoBar::showComponent(const Component & comp, std::string message)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	state = COMPONENT;
	visibleInfo = std::make_shared<VisibleComponentInfo>(comp, message);
	setTimer(3000);
	redraw();
}

void CInfoBar::startEnemyTurn(PlayerColor color)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	state = AITURN;
	visibleInfo = std::make_shared<VisibleEnemyTurnInfo>(color);
	redraw();
}

void CInfoBar::showHeroSelection(const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	if(!hero)
	{
		reset();
	}
	else
	{
		state = HERO;
		visibleInfo = std::make_shared<VisibleHeroInfo>(hero);
	}
	redraw();
}

void CInfoBar::showTownSelection(const CGTownInstance * town)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	if(!town)
	{
		reset();
	}
	else
	{
		state = TOWN;
		visibleInfo = std::make_shared<VisibleTownInfo>(town);
	}
	redraw();
}

void CInfoBar::showGameStatus()
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	state = GAME;
	visibleInfo = std::make_shared<VisibleGameStatusInfo>();
	setTimer(3000);
	redraw();
}

CInGameConsole::CInGameConsole()
	: CIntObject(KEYBOARD | TEXTINPUT),
	prevEntDisp(-1),
	defaultTimeout(10000),
	maxDisplayedTexts(10)
{
}

void CInGameConsole::show(SDL_Surface * to)
{
	int number = 0;

	std::vector<std::list< std::pair< std::string, uint32_t > >::iterator> toDel;

	boost::unique_lock<boost::mutex> lock(texts_mx);
	for(auto it = texts.begin(); it != texts.end(); ++it, ++number)
	{
		Point leftBottomCorner(0, pos.h);

		graphics->fonts[FONT_MEDIUM]->renderTextLeft(to, it->first, Colors::GREEN,
			Point(leftBottomCorner.x + 50, leftBottomCorner.y - (int)texts.size() * 20 - 80 + number*20));

		if((int)(SDL_GetTicks() - it->second) > defaultTimeout)
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
				endEnteringText(false);
			}
			else if(SDLK_TAB == key.keysym.sym)
			{
				startEnteringText();
			}
			break;
		}
	case SDLK_RETURN: //enter key
		{
			if(!enteredText.empty() && captureAllKeys)
			{
				bool anyTextExceptCaret = enteredText.size() > 1;
				endEnteringText(anyTextExceptCaret);

				if(anyTextExceptCaret)
				{
					CCS->soundh->playSound("CHAT");
				}
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
				prevEntDisp = static_cast<int>(previouslyEntered.size() - 1);
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
	if (!active)
		return;

	if (captureAllKeys)
		return;

	assert(GH.statusbar);
	assert(currentStatusBar.expired());//effectively, nullptr check

	currentStatusBar = GH.statusbar;

	captureAllKeys = true;
	enteredText = "_";

	GH.statusbar->setEnteringMode(true);
	GH.statusbar->setEnteredText(enteredText);
}

void CInGameConsole::endEnteringText(bool processEnteredText)
{
	captureAllKeys = false;
	prevEntDisp = -1;
	if(processEnteredText)
	{
		std::string txt = enteredText.substr(0, enteredText.size()-1);
		previouslyEntered.push_back(txt);

		if(txt.at(0) == '/')
		{
			//some commands like gosolo don't work when executed from GUI thread
			auto threadFunction = [=]()
			{
				ClientCommandManager commandController;
				commandController.processCommand(txt.substr(1), true);
			};

			boost::thread clientCommandThread(threadFunction);
			clientCommandThread.detach();
		}
		else
			LOCPLINT->cb->sendMessage(txt, LOCPLINT->getSelection());
	}
	enteredText.clear();

	auto statusbar = currentStatusBar.lock();
	assert(statusbar);

	if (statusbar)
		statusbar->setEnteringMode(false);

	currentStatusBar.reset();
}

void CInGameConsole::refreshEnteredText()
{
	auto statusbar = currentStatusBar.lock();
	assert(statusbar);

	if (statusbar)
		statusbar->setEnteredText(enteredText);
}

CAdvMapPanel::CAdvMapPanel(std::shared_ptr<IImage> bg, Point position)
	: CIntObject()
	, background(bg)
{
	defActions = 255;
	recActions = 255;
	pos.x += position.x;
	pos.y += position.y;
	if (bg)
	{
		pos.w = bg->width();
		pos.h = bg->height();
	}
}

void CAdvMapPanel::addChildColorableButton(std::shared_ptr<CButton> button)
{
	colorableButtons.push_back(button);
	addChildToPanel(button, ACTIVATE | DEACTIVATE);
}

void CAdvMapPanel::setPlayerColor(const PlayerColor & clr)
{
	for(auto & button : colorableButtons)
	{
		button->setPlayerColor(clr);
	}
}

void CAdvMapPanel::showAll(SDL_Surface * to)
{
	if(background)
		background->draw(to, pos.x, pos.y);

	CIntObject::showAll(to);
}

void CAdvMapPanel::addChildToPanel(std::shared_ptr<CIntObject> obj, ui8 actions)
{
	otherObjects.push_back(obj);
	obj->recActions |= actions | SHOWALL;
	obj->recActions &= ~DISPOSE;
	addChild(obj.get(), false);
}

CAdvMapWorldViewPanel::CAdvMapWorldViewPanel(std::shared_ptr<CAnimation> _icons, std::shared_ptr<IImage> bg, Point position, int spaceBottom, const PlayerColor &color)
	: CAdvMapPanel(bg, position), icons(_icons)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	int fillerHeight = bg ? spaceBottom - pos.y - pos.h : 0;

	if(fillerHeight > 0)
	{
		backgroundFiller = std::make_shared<CFilledTexture>("DIBOXBCK", Rect(0, pos.h, pos.w, fillerHeight));
	}
}

CAdvMapWorldViewPanel::~CAdvMapWorldViewPanel() = default;

void CAdvMapWorldViewPanel::recolorIcons(const PlayerColor & color, int indexOffset)
{
	assert(iconsData.size() == currentIcons.size());

	for(size_t idx = 0; idx < iconsData.size(); idx++)
	{
		const auto & data = iconsData.at(idx);
		currentIcons[idx]->setFrame(data.first + indexOffset);
	}
}

void CAdvMapWorldViewPanel::addChildIcon(std::pair<int, Point> data, int indexOffset)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	iconsData.push_back(data);
	currentIcons.push_back(std::make_shared<CAnimImage>(icons, data.first + indexOffset, 0, data.second.x, data.second.y));
}
