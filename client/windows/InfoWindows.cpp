/*
 * InfoWindows.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "InfoWindows.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../PlayerLocalState.h"

#include "../adventureMap/AdventureMapInterface.h"
#include "../adventureMap/CMinimap.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/TextControls.h"
#include "../windows/CMessage.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/gameState/InfoAboutArmy.h"
#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/CQuest.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../ConditionalWait.h"

CSelWindow::CSelWindow( const std::string & Text, PlayerColor player, int charperline, const std::vector<std::shared_ptr<CSelectableComponent>> & comps, const std::vector<std::pair<AnimationPath, CFunctionList<void()>>> & Buttons, QueryID askID)
{
	OBJECT_CONSTRUCTION;

	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DiBoxBck"), pos);

	ID = askID;
	for(int i = 0; i < Buttons.size(); i++)
	{
		buttons.push_back(std::make_shared<CButton>(Point(0, 0), Buttons[i].first, CButton::tooltip(), Buttons[i].second));
		if(!i && askID.getNum() >= 0)
			buttons.back()->addCallback(std::bind(&CSelWindow::madeChoice, this));
		buttons[i]->addCallback(std::bind(&CInfoWindow::close, this)); //each button will close the window apart from call-defined actions
	}

	text = std::make_shared<CTextBox>(Text, Rect(0, 0, 250, 100), 0, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE);

	if(buttons.size() > 1 && askID.getNum() >= 0) //cancel button functionality
		buttons.back()->addCallback([askID](){LOCPLINT->cb->selectionMade(0, askID);});

	if(buttons.size() == 1)
		buttons.front()->assignedKey = EShortcut::GLOBAL_RETURN;

	if(buttons.size() == 2)
	{
		buttons.front()->assignedKey = EShortcut::GLOBAL_ACCEPT;
		buttons.back()->assignedKey = EShortcut::GLOBAL_CANCEL;
	}

	if(!comps.empty())
	{
		components = std::make_shared<CComponentBox>(comps, Rect(0,0,0,0));
		for (auto & comp : comps)
			comp->onChoose = [this](){ madeChoiceAndClose(); };
	}

	CMessage::drawIWindow(this, Text, player);
}

void CSelWindow::madeChoice()
{
	if(ID.getNum() < 0)
		return;
	int ret = -1;
	if(components)
		ret = components->selectedIndex();

	LOCPLINT->cb->selectionMade(ret + 1, ID);
}

void CSelWindow::madeChoiceAndClose()
{
	madeChoice();
	close();
}

CInfoWindow::CInfoWindow(const std::string & Text, PlayerColor player, const TCompsInfo & comps, const TButtonsInfo & Buttons)
{
	OBJECT_CONSTRUCTION;

	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DiBoxBck"), pos);

	ID = QueryID(-1);
	for(const auto & Button : Buttons)
	{
		auto button = std::make_shared<CButton>(Point(0, 0), Button.first, CButton::tooltip(), std::bind(&CInfoWindow::close, this));
		button->setBorderColor(Colors::METALLIC_GOLD);
		button->addCallback(Button.second); //each button will close the window apart from call-defined actions
		buttons.push_back(button);
	}

	text = std::make_shared<CTextBox>(Text, Rect(0, 0, 250, 100), 0, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE);
	if(!text->slider)
	{
		int finalWidth = std::min(250, text->label->textSize.x + 32);
		int finalHeight = text->label->textSize.y;
		text->resize(Point(finalWidth, finalHeight));
	}

	if(buttons.size() == 1)
		buttons.front()->assignedKey = EShortcut::GLOBAL_RETURN;

	if(buttons.size() == 2)
	{
		buttons.front()->assignedKey = EShortcut::GLOBAL_ACCEPT;
		buttons.back()->assignedKey = EShortcut::GLOBAL_CANCEL;
	}

	if(!comps.empty())
		components = std::make_shared<CComponentBox>(comps, Rect(0,0,0,0));

	CMessage::drawIWindow(this, Text, player);
}

CInfoWindow::CInfoWindow()
{
	ID = QueryID(-1);
}

void CInfoWindow::close()
{
	WindowBase::close();

	if(LOCPLINT)
		LOCPLINT->showingDialog->setFree();
}

void CInfoWindow::showAll(Canvas & to)
{
	CIntObject::showAll(to);
	CMessage::drawBorder(LOCPLINT ? LOCPLINT->playerID : PlayerColor(1), to, pos.w+28, pos.h+29, pos.x-14, pos.y-15);
}

CInfoWindow::~CInfoWindow() = default;

void CInfoWindow::showInfoDialog(const std::string & text, const TCompsInfo & components, PlayerColor player)
{
	GH.windows().pushWindow(CInfoWindow::create(text, player, components));
}

void CInfoWindow::showYesNoDialog(const std::string & text, const TCompsInfo & components, const CFunctionList<void()> & onYes, const CFunctionList<void()> & onNo, PlayerColor player)
{
	assert(!LOCPLINT || LOCPLINT->showingDialog->isBusy());
	std::vector<std::pair<AnimationPath, CFunctionList<void()>>> pom;
	pom.emplace_back(AnimationPath::builtin("IOKAY.DEF"), nullptr);
	pom.emplace_back(AnimationPath::builtin("ICANCEL.DEF"), nullptr);
	auto temp = std::make_shared<CInfoWindow>(text, player, components, pom);

	temp->buttons[0]->addCallback(onYes);
	temp->buttons[1]->addCallback(onNo);

	GH.windows().pushWindow(temp);
}

std::shared_ptr<CInfoWindow> CInfoWindow::create(const std::string & text, PlayerColor playerID, const TCompsInfo & components)
{
	std::vector<std::pair<AnimationPath, CFunctionList<void()>>> pom;
	pom.emplace_back(AnimationPath::builtin("IOKAY.DEF"), nullptr);
	return std::make_shared<CInfoWindow>(text, playerID, components, pom);
}

std::string CInfoWindow::genText(const std::string & title, const std::string & description)
{
	return std::string("{") + title + "}" + "\n\n" + description;
}

bool CRClickPopup::isPopupWindow() const
{
	return true;
}

void CRClickPopup::createAndPush(const std::string & txt, const CInfoWindow::TCompsInfo & comps)
{
	PlayerColor player = LOCPLINT ? LOCPLINT->playerID : PlayerColor(1); //if no player, then use blue
	if(settings["session"]["spectate"].Bool()) //TODO: there must be better way to implement this
		player = PlayerColor(1);

	auto temp = std::make_shared<CInfoWindow>(txt, player, comps);
	temp->center(GH.getCursorPosition()); //center on mouse
#ifdef VCMI_MOBILE
	temp->moveBy({0, -temp->pos.h / 2});
#endif
	temp->fitToScreen(10);

	GH.windows().createAndPushWindow<CRClickPopupInt>(temp);
}

void CRClickPopup::createAndPush(const std::string & txt, const std::shared_ptr<CComponent> & component)
{
	CInfoWindow::TCompsInfo intComps;
	intComps.push_back(component);

	createAndPush(txt, intComps);
}

void CRClickPopup::createAndPush(const CGObjectInstance * obj, const Point & p, ETextAlignment alignment)
{
	auto iWin = createCustomInfoWindow(p, obj); //try get custom infowindow for this obj
	if(iWin)
	{
		GH.windows().pushWindow(iWin);
	}
	else
	{
		std::vector<Component> components;
		if(settings["general"]["enableUiEnhancements"].Bool())
		{
			if(LOCPLINT->localState->getCurrentHero())
				components = obj->getPopupComponents(LOCPLINT->localState->getCurrentHero());
			else
				components = obj->getPopupComponents(LOCPLINT->playerID);
		}

		std::vector<std::shared_ptr<CComponent>> guiComponents;
		for(auto & component : components)
			guiComponents.push_back(std::make_shared<CComponent>(component));

		if(LOCPLINT->localState->getCurrentHero())
			CRClickPopup::createAndPush(obj->getPopupText(LOCPLINT->localState->getCurrentHero()), guiComponents);
		else
			CRClickPopup::createAndPush(obj->getPopupText(LOCPLINT->playerID), guiComponents);
	}
}

CRClickPopupInt::CRClickPopupInt(const std::shared_ptr<CIntObject> & our) :
	dragDistance(Point(0, 0))
{
	addUsedEvents(DRAG_POPUP);

	CCS->curh->hide();
	inner = our;
	addChild(our.get(), false);
}

CRClickPopupInt::~CRClickPopupInt()
{
	CCS->curh->show();
}

void CRClickPopupInt::mouseDraggedPopup(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	if(!settings["adventure"]["rightButtonDrag"].Bool())
		return;
	
	dragDistance += lastUpdateDistance;
	
	if(dragDistance.length() > 16)
		close();
}

void CInfoBoxPopup::mouseDraggedPopup(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	if(!settings["adventure"]["rightButtonDrag"].Bool())
		return;
	
	dragDistance += lastUpdateDistance;
	
	if(dragDistance.length() > 16)
		close();
}


CInfoBoxPopup::CInfoBoxPopup(Point position, const CGTownInstance * town)
	: CWindowObject(RCLICK_POPUP | PLAYER_COLORED, ImagePath::builtin("TOWNQVBK"), position)
{
	InfoAboutTown iah;
	LOCPLINT->cb->getTownInfo(town, iah, LOCPLINT->localState->getCurrentArmy()); //todo: should this be nearest hero?

	OBJECT_CONSTRUCTION;
	tooltip = std::make_shared<CTownTooltip>(Point(9, 10), iah);

	addUsedEvents(DRAG_POPUP);

	fitToScreen(10);
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGHeroInstance * hero)
	: CWindowObject(RCLICK_POPUP | PLAYER_COLORED, ImagePath::builtin("HEROQVBK"), position)
{
	InfoAboutHero iah;
	LOCPLINT->cb->getHeroInfo(hero, iah, LOCPLINT->localState->getCurrentArmy()); //todo: should this be nearest hero?

	OBJECT_CONSTRUCTION;
	tooltip = std::make_shared<CHeroTooltip>(Point(9, 10), iah);
	
	addUsedEvents(DRAG_POPUP);

	fitToScreen(10);
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGGarrison * garr)
	: CWindowObject(RCLICK_POPUP | PLAYER_COLORED, ImagePath::builtin("TOWNQVBK"), position)
{
	InfoAboutTown iah;
	LOCPLINT->cb->getTownInfo(garr, iah);

	OBJECT_CONSTRUCTION;
	tooltip = std::make_shared<CArmyTooltip>(Point(9, 10), iah);
	
	addUsedEvents(DRAG_POPUP);

	fitToScreen(10);
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGCreature * creature)
	: CWindowObject(RCLICK_POPUP | BORDERED, ImagePath::builtin("DIBOXBCK"), position)
{
	OBJECT_CONSTRUCTION;
	tooltip = std::make_shared<CreatureTooltip>(Point(9, 10), creature);
	
	addUsedEvents(DRAG_POPUP);

	fitToScreen(10);
}

MinimapWithIcons::MinimapWithIcons(const Point & position)
{
	OBJECT_CONSTRUCTION;
	pos += position;

	Rect areaSurface(11, 41, 144, 144);
	Rect areaUnderground(167, 41, 144, 144);

	Rect borderSurface(10, 40, 147, 147);
	Rect borderUnderground(166, 40, 147, 147);

	bool singleLevelMap = LOCPLINT->cb->getMapSize().z == 1;

	if (singleLevelMap)
	{
		areaSurface.x += 78;
		borderSurface.x += 78;
	}

	backgroundSurface = std::make_shared<TransparentFilledRectangle>(borderSurface, Colors::TRANSPARENCY, Colors::YELLOW);
	surface = std::make_shared<CMinimapInstance>(areaSurface.topLeft(), areaSurface.dimensions(), 0);

	if (!singleLevelMap)
	{
		backgroundUnderground = std::make_shared<TransparentFilledRectangle>(borderUnderground, Colors::TRANSPARENCY, Colors::YELLOW);
		undergroud = std::make_shared<CMinimapInstance>(areaUnderground.topLeft(), areaUnderground.dimensions(), 1);
	}
}

void MinimapWithIcons::addIcon(const int3 & coordinates, const ImagePath & image )
{
	OBJECT_CONSTRUCTION;

	Rect areaSurface(11, 41, 144, 144);
	Rect areaUnderground(167, 41, 144, 144);
	bool singleLevelMap = LOCPLINT->cb->getMapSize().z == 1;
	if (singleLevelMap)
		areaSurface.x += 78;

	int positionX = 144 * coordinates.x / LOCPLINT->cb->getMapSize().x;
	int positionY = 144 * coordinates.y / LOCPLINT->cb->getMapSize().y;

	Point iconPosition(positionX, positionY);

	iconPosition -= Point(8,8); // compensate for 16x16 icon half-size

	if (coordinates.z == 0)
		iconPosition += areaSurface.topLeft();
	else
		iconPosition += areaUnderground.topLeft();

	iconsOverlay.push_back(std::make_shared<CPicture>(image, iconPosition));
}

TeleporterPopup::TeleporterPopup(const Point & position, const CGTeleport * teleporter)
	: CWindowObject(BORDERED | RCLICK_POPUP)
{
	OBJECT_CONSTRUCTION;
	pos.w = 322;
	pos.h = 200;

	filledBackground = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));
	labelTitle = std::make_shared<CLabel>(pos.w / 2, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, teleporter->getPopupText(LOCPLINT->playerID));
	minimap = std::make_shared<MinimapWithIcons>(Point(0,0));

	const auto & entrances = teleporter->getAllEntrances();
	const auto & exits = teleporter->getAllExits();

	std::set<ObjectInstanceID> allTeleporters;
	allTeleporters.insert(entrances.begin(), entrances.end());
	allTeleporters.insert(exits.begin(), exits.end());

	for (const auto exit : allTeleporters)
	{
		const auto * exitObject = LOCPLINT->cb->getObj(exit, false);

		if (!exitObject)
			continue;

		int3 position = exitObject->visitablePos();
		ImagePath image;

		if (!vstd::contains(entrances, exit))
			image = ImagePath::builtin("portalExit");
		else if (!vstd::contains(exits, exit))
			image = ImagePath::builtin("portalEntrance");
		else
			image = ImagePath::builtin("portalBidirectional");

		minimap->addIcon(position, image);
	}
	center(position);
	fitToScreen(10);
}

KeymasterPopup::KeymasterPopup(const Point & position, const CGKeys * keymasterOrGuard)
	: CWindowObject(BORDERED | RCLICK_POPUP)
{
	OBJECT_CONSTRUCTION;
	pos.w = 322;
	pos.h = 220;

	filledBackground = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));
	labelTitle = std::make_shared<CLabel>(pos.w / 2, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, keymasterOrGuard->getObjectName());
	labelDescription = std::make_shared<CLabel>(pos.w / 2, 40, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, keymasterOrGuard->getObjectDescription(LOCPLINT->playerID));
	minimap = std::make_shared<MinimapWithIcons>(Point(0,20));

	const auto allObjects = LOCPLINT->cb->getAllVisitableObjs();

	for (const auto mapObject : allObjects)
	{
		if (!mapObject)
			continue;

		switch (mapObject->ID)
		{
			case Obj::KEYMASTER:
				if (mapObject->subID == keymasterOrGuard->subID)
					minimap->addIcon(mapObject->visitablePos(), ImagePath::builtin("keymaster"));
				break;
			case Obj::BORDERGUARD:
				if (mapObject->subID == keymasterOrGuard->subID)
					minimap->addIcon(mapObject->visitablePos(), ImagePath::builtin("borderguard"));
				break;
			case Obj::BORDER_GATE:
				if (mapObject->subID == keymasterOrGuard->subID)
					minimap->addIcon(mapObject->visitablePos(), ImagePath::builtin("bordergate"));
				break;
		}
	}
	center(position);
	fitToScreen(10);
}

std::shared_ptr<WindowBase>
CRClickPopup::createCustomInfoWindow(Point position, const CGObjectInstance * specific) //specific=0 => draws info about selected town/hero
{
	if(nullptr == specific)
		specific = LOCPLINT->localState->getCurrentArmy();

	if(nullptr == specific)
	{
		logGlobal->error("createCustomInfoWindow: no object to describe");
		return nullptr;
	}

	switch(specific->ID)
	{
		case Obj::HERO:
			return std::make_shared<CInfoBoxPopup>(position, dynamic_cast<const CGHeroInstance *>(specific));
		case Obj::TOWN:
			return std::make_shared<CInfoBoxPopup>(position, dynamic_cast<const CGTownInstance *>(specific));
		case Obj::MONSTER:
			return std::make_shared<CInfoBoxPopup>(position, dynamic_cast<const CGCreature *>(specific));
		case Obj::GARRISON:
		case Obj::GARRISON2:
			return std::make_shared<CInfoBoxPopup>(position, dynamic_cast<const CGGarrison *>(specific));
		case Obj::MONOLITH_ONE_WAY_ENTRANCE:
		case Obj::MONOLITH_ONE_WAY_EXIT:
		case Obj::MONOLITH_TWO_WAY:
		case Obj::SUBTERRANEAN_GATE:
		case Obj::WHIRLPOOL:
			return std::make_shared<TeleporterPopup>(position, dynamic_cast<const CGTeleport *>(specific));
		case Obj::KEYMASTER:
		case Obj::BORDERGUARD:
		case Obj::BORDER_GATE:
			return std::make_shared<KeymasterPopup>(position, dynamic_cast<const CGKeys *>(specific));
		default:
			return std::shared_ptr<WindowBase>();
	}
}
