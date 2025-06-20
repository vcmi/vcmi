/*
 * CKingdomInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CKingdomInterface.h"

#include "CCastleInterface.h"
#include "CPlayerState.h"
#include "InfoWindows.h"

#include "../CPlayerInterface.h"
#include "../PlayerLocalState.h"
#include "../adventureMap/CResDataBar.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/CGarrisonInt.h"
#include "../widgets/TextControls.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/Buttons.h"
#include "../widgets/ObjectLists.h"
#include "../windows/CMarketWindow.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/StartInfo.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/texts/TextOperations.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "texts/CGeneralTextHandler.h"
#include "../../lib/GameSettings.h"

static const std::string OVERVIEW_BACKGROUND = "OvCast.pcx";
static const size_t OVERVIEW_SIZE = 4;

InfoBox::InfoBox(Point position, InfoPos Pos, InfoSize Size, std::shared_ptr<IInfoBoxData> Data):
	size(Size),
	infoPos(Pos),
	data(Data),
	value(nullptr),
	name(nullptr)
{
	assert(data);
	addUsedEvents(LCLICK | SHOW_POPUP);
	EFonts font = (size < SIZE_MEDIUM)? FONT_SMALL: FONT_MEDIUM;

	OBJECT_CONSTRUCTION;
	pos+=position;

	image = std::make_shared<CAnimImage>(data->getImageName(size), data->getImageIndex());
	pos = image->pos;

	switch(infoPos)
	{
	case POS_CORNER:
		value = std::make_shared<CLabel>(pos.w, pos.h, font, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, data->getValueText());
		break;
	case POS_INSIDE:
		value = std::make_shared<CLabel>(pos.w/2, pos.h-6, font, ETextAlignment::CENTER, Colors::WHITE, data->getValueText());
		break;
	case POS_UP_DOWN:
		name = std::make_shared<CLabel>(pos.w/2, -12, font, ETextAlignment::CENTER, Colors::WHITE, data->getNameText());
		[[fallthrough]];
	case POS_DOWN:
		value = std::make_shared<CLabel>(pos.w/2, pos.h+8, font, ETextAlignment::CENTER, Colors::WHITE, data->getValueText());
		break;
	case POS_RIGHT:
		name = std::make_shared<CLabel>(pos.w+6, 6, font, ETextAlignment::TOPLEFT, Colors::WHITE, data->getNameText());
		value = std::make_shared<CLabel>(pos.w+6, pos.h-16, font, ETextAlignment::TOPLEFT, Colors::WHITE, data->getValueText());
		break;
	}

	if(name)
		pos = pos.include(name->pos);
	if(value)
		pos = pos.include(value->pos);

	hover = std::make_shared<CHoverableArea>();
	hover->hoverText = data->getHoverText();
	hover->pos = pos;
}

InfoBox::~InfoBox() = default;

void InfoBox::showPopupWindow(const Point & cursorPosition)
{
	std::shared_ptr<CComponent> comp;
	std::string text;
	data->prepareMessage(text, comp);
	if (comp)
		CRClickPopup::createAndPush(text, CInfoWindow::TCompsInfo(1, comp));
	else if (!text.empty())
		CRClickPopup::createAndPush(text);
}

void InfoBox::clickPressed(const Point & cursorPosition)
{
	std::shared_ptr<CComponent> comp;
	std::string text;
	data->prepareMessage(text, comp);

	if(comp)
		GAME->interface()->showInfoDialog(text, CInfoWindow::TCompsInfo(1, comp));
}

IInfoBoxData::IInfoBoxData(InfoType Type)
	: type(Type)
{
}

InfoBoxAbstractHeroData::InfoBoxAbstractHeroData(InfoType Type)
	: IInfoBoxData(Type)
{
}

std::string InfoBoxAbstractHeroData::getValueText()
{
	switch (type)
	{
	case HERO_MANA:
	case HERO_PRIMARY_SKILL:
		return std::to_string(getValue());
	case HERO_EXPERIENCE:
		return TextOperations::formatMetric(getValue(), 6);
	case HERO_SPECIAL:
		return LIBRARY->generaltexth->jktexts[5];
	case HERO_SECONDARY_SKILL:
		{
			si64 value = getValue();
			if (value)
				return LIBRARY->generaltexth->levels[value];
			else
				return "";
		}
	default:
		logGlobal->error("Invalid InfoBox info type");
	}
	return "";
}

std::string InfoBoxAbstractHeroData::getNameText()
{
	switch (type)
	{
	case HERO_PRIMARY_SKILL:
		return LIBRARY->generaltexth->primarySkillNames[getSubID()];
	case HERO_MANA:
		return LIBRARY->generaltexth->allTexts[387];
	case HERO_EXPERIENCE:
		return LIBRARY->generaltexth->jktexts[6];
	case HERO_SPECIAL:
		return LIBRARY->heroh->objects[getSubID()]->getSpecialtyNameTranslated();
	case HERO_SECONDARY_SKILL:
		if (getValue())
			return LIBRARY->skillh->getByIndex(getSubID())->getNameTranslated();
		else
			return "";
	default:
		logGlobal->error("Invalid InfoBox info type");
	}
	return "";
}

AnimationPath InfoBoxAbstractHeroData::getImageName(InfoBox::InfoSize size)
{
	//TODO: sizes
	switch(size)
	{
	case InfoBox::SIZE_SMALL:
		{
			switch(type)
			{
			case HERO_PRIMARY_SKILL:
			case HERO_MANA:
			case HERO_EXPERIENCE:
				return AnimationPath::builtin("PSKIL32");
			case HERO_SPECIAL:
				return AnimationPath::builtin("UN32");
			case HERO_SECONDARY_SKILL:
				return AnimationPath::builtin("SECSK32");
			default:
				assert(0);
			}
		}
	case InfoBox::SIZE_BIG:
		{
			switch(type)
			{
			case HERO_PRIMARY_SKILL:
			case HERO_MANA:
			case HERO_EXPERIENCE:
				return AnimationPath::builtin("PSKIL42");
			case HERO_SPECIAL:
				return AnimationPath::builtin("UN44");
			case HERO_SECONDARY_SKILL:
				return AnimationPath::builtin("SECSKILL");
			default:
				assert(0);
			}
		}
	default:
		assert(0);
	}
	return {};
}

std::string InfoBoxAbstractHeroData::getHoverText()
{
	//TODO: any texts here?
	return "";
}

size_t InfoBoxAbstractHeroData::getImageIndex()
{
	switch (type)
	{
	case HERO_SPECIAL:
		return LIBRARY->heroh->objects[getSubID()]->imageIndex;
	case HERO_PRIMARY_SKILL:
		return getSubID();
	case HERO_MANA:
		return 5;
	case HERO_EXPERIENCE:
		return 4;
	case HERO_SECONDARY_SKILL:
		{
			si64 value = getValue();
			if (value)
				return getSubID()*3 + value + 2;
			else
				return 0;//FIXME: Should be transparent instead of empty
		}
	default:
		assert(0);
		return 0;
	}
}

void InfoBoxAbstractHeroData::prepareMessage(std::string & text, std::shared_ptr<CComponent> & comp)
{
	comp.reset();
	switch (type)
	{
	case HERO_SPECIAL:
		text = LIBRARY->heroh->objects[getSubID()]->getSpecialtyDescriptionTranslated();
		break;
	case HERO_PRIMARY_SKILL:
		text = LIBRARY->generaltexth->arraytxt[2+getSubID()];
		comp = std::make_shared<CComponent>(ComponentType::PRIM_SKILL, PrimarySkill(getSubID()), getValue());
		break;
	case HERO_MANA:
		text = LIBRARY->generaltexth->allTexts[149];
		break;
	case HERO_EXPERIENCE:
		text = LIBRARY->generaltexth->allTexts[241];
		break;
	case HERO_SECONDARY_SKILL:
		{
			si64 value = getValue();
			int  subID = getSubID();
			if(value)
			{
				text = LIBRARY->skillh->getByIndex(subID)->getDescriptionTranslated((int)value);
				comp = std::make_shared<CComponent>(ComponentType::SEC_SKILL, SecondarySkill(subID), (int)value);
			}
			break;
		}
	default:
		break;
	}
}

InfoBoxHeroData::InfoBoxHeroData(InfoType Type, const CGHeroInstance * Hero, int Index):
	InfoBoxAbstractHeroData(Type),
	hero(Hero),
	index(Index)
{
}

int InfoBoxHeroData::getSubID()
{
	switch(type)
	{
	case HERO_PRIMARY_SKILL:
		return index;
	case HERO_SECONDARY_SKILL:
		if(hero->secSkills.size() > index)
			return hero->secSkills[index].first.getNum();
		else
			return 0;
	case HERO_SPECIAL:
		return hero->getHeroTypeID().getNum();
	case HERO_MANA:
	case HERO_EXPERIENCE:
		return 0;
	default:
		assert(0);
		return 0;
	}
}

si64 InfoBoxHeroData::getValue()
{
	if(!hero)
		return 0;

	switch(type)
	{
	case HERO_PRIMARY_SKILL:
		return hero->getPrimSkillLevel(static_cast<PrimarySkill>(index));
	case HERO_MANA:
		return hero->mana;
	case HERO_EXPERIENCE:
		return hero->exp;
	case HERO_SECONDARY_SKILL:
		if(hero->secSkills.size() > index)
			return hero->secSkills[index].second;
		else
			return 0;
	case HERO_SPECIAL:
		return 0;
	default:
		assert(0);
		return 0;
	}
}

std::string InfoBoxHeroData::getHoverText()
{
	switch (type)
	{
	case HERO_PRIMARY_SKILL:
		return boost::str(boost::format(LIBRARY->generaltexth->heroscrn[1]) % LIBRARY->generaltexth->primarySkillNames[index]);
	case HERO_MANA:
		return LIBRARY->generaltexth->heroscrn[22];
	case HERO_EXPERIENCE:
		return LIBRARY->generaltexth->heroscrn[9];
	case HERO_SPECIAL:
		return LIBRARY->generaltexth->heroscrn[27];
	case HERO_SECONDARY_SKILL:
		if (hero->secSkills.size() > index)
		{
			std::string level = LIBRARY->generaltexth->levels[hero->secSkills[index].second-1];
			std::string skill = hero->secSkills[index].first.toEntity(LIBRARY)->getNameTranslated();
			return boost::str(boost::format(LIBRARY->generaltexth->heroscrn[21]) % level % skill);
		}
		else
		{
			return "";
		}
	default:
		return InfoBoxAbstractHeroData::getHoverText();
	}
}

std::string InfoBoxHeroData::getValueText()
{
	if (hero)
	{
		switch (type)
		{
		case HERO_MANA:
			return std::to_string(hero->mana) + '/' +
				std::to_string(hero->manaLimit());
		case HERO_EXPERIENCE:
			return TextOperations::formatMetric(hero->exp, 6);
		}
	}
	return InfoBoxAbstractHeroData::getValueText();
}

void InfoBoxHeroData::prepareMessage(std::string & text, std::shared_ptr<CComponent> & comp)
{
	comp.reset();
	switch(type)
	{
	case HERO_MANA:
		text = LIBRARY->generaltexth->allTexts[205];
		boost::replace_first(text, "%s", hero->getNameTranslated());
		boost::replace_first(text, "%d", std::to_string(hero->mana));
		boost::replace_first(text, "%d", std::to_string(hero->manaLimit()));
		break;
	case HERO_EXPERIENCE:
		text = LIBRARY->generaltexth->allTexts[2];
		boost::replace_first(text, "%d", std::to_string(hero->level));
		boost::replace_first(text, "%d", std::to_string(LIBRARY->heroh->reqExp(hero->level+1)));
		boost::replace_first(text, "%d", std::to_string(hero->exp));
		break;
	default:
		InfoBoxAbstractHeroData::prepareMessage(text, comp);
		break;
	}
}

InfoBoxCustomHeroData::InfoBoxCustomHeroData(InfoType Type, int SubID, si64 Value):
	InfoBoxAbstractHeroData(Type),
	subID(SubID),
	value(Value)
{
}

int InfoBoxCustomHeroData::getSubID()
{
	return subID;
}

si64 InfoBoxCustomHeroData::getValue()
{
	return value;
}

InfoBoxCustom::InfoBoxCustom(std::string ValueText, std::string NameText, const AnimationPath & ImageName, size_t ImageIndex, std::string HoverText):
	IInfoBoxData(CUSTOM),
	valueText(ValueText),
	nameText(NameText),
	imageName(ImageName),
	hoverText(HoverText),
	imageIndex(ImageIndex)
{
}

std::string InfoBoxCustom::getHoverText()
{
	return hoverText;
}

size_t InfoBoxCustom::getImageIndex()
{
	return imageIndex;
}

AnimationPath InfoBoxCustom::getImageName(InfoBox::InfoSize size)
{
	return imageName;
}

std::string InfoBoxCustom::getNameText()
{
	return nameText;
}

std::string InfoBoxCustom::getValueText()
{
	return valueText;
}

void InfoBoxCustom::prepareMessage(std::string & text, std::shared_ptr<CComponent> & comp)
{
}

CKingdomInterface::CKingdomInterface()
	: CWindowObject(PLAYER_COLORED | BORDERED, ImagePath::builtin(OVERVIEW_BACKGROUND))
{
	OBJECT_CONSTRUCTION;
	ui32 footerPos = OVERVIEW_SIZE * 116;

	tabArea = std::make_shared<CTabbedInt>(std::bind(&CKingdomInterface::createMainTab, this, _1), Point(4,4));

	std::vector<const CGObjectInstance * > ownedObjects = GAME->interface()->cb->getMyObjects();
	generateObjectsList(ownedObjects);
	generateMinesList(ownedObjects);
	generateButtons();

	statusbar = CGStatusBar::create(std::make_shared<CPicture>(ImagePath::builtin("KSTATBAR"), 10,pos.h - 45));
	resdatabar = std::make_shared<CResDataBar>(ImagePath::builtin("KRESBAR"), 7, 111+footerPos, 29, 3, 76, 81);

	activateTab(settings["general"]["lastKindomInterface"].Integer());
}

void CKingdomInterface::generateObjectsList(const std::vector<const CGObjectInstance * > &ownedObjects)
{
	ui32 footerPos = OVERVIEW_SIZE * 116;
	size_t dwellSize = (footerPos - 64)/57;

	//Map used to determine image number for several objects
	std::map<std::pair<int,int>,int> idToImage;
	idToImage[std::make_pair( 20, 1)] = 81;//Golem factory
	idToImage[std::make_pair( 42, 0)] = 82;//Lighthouse
	idToImage[std::make_pair( 33, 0)] = 83;//Garrison
	idToImage[std::make_pair(219, 0)] = 83;//Garrison
	idToImage[std::make_pair( 33, 1)] = 84;//Anti-magic Garrison
	idToImage[std::make_pair(219, 1)] = 84;//Anti-magic Garrison
	idToImage[std::make_pair( 53, 7)] = 85;//Abandoned mine
	idToImage[std::make_pair( 20, 0)] = 86;//Conflux
	idToImage[std::make_pair( 87, 0)] = 87;//Harbor

	std::map<int, OwnedObjectInfo> visibleObjects;
	for(const CGObjectInstance * object : ownedObjects)
	{
		//Dwellings
		if(object->ID == Obj::CREATURE_GENERATOR1)
		{
			OwnedObjectInfo & info = visibleObjects[object->subID];
			if(info.count++ == 0)
			{
				info.hoverText = object->getObjectName();
				info.imageID = object->subID;
			}
		}
		//Special objects from idToImage map that should be displayed in objects list
		auto iter = idToImage.find(std::make_pair(object->ID, object->subID));
		if(iter != idToImage.end())
		{
			OwnedObjectInfo & info = visibleObjects[iter->second];
			if(info.count++ == 0)
			{
				info.hoverText = object->getObjectName();
				info.imageID = iter->second;
			}
		}
	}
	objects.reserve(visibleObjects.size());

	for(auto & element : visibleObjects)
	{
		objects.push_back(element.second);
	}
	dwellingsList = std::make_shared<CListBox>(std::bind(&CKingdomInterface::createOwnedObject, this, _1),
		Point(740,44), Point(0,57), dwellSize, visibleObjects.size());
}

std::shared_ptr<CIntObject> CKingdomInterface::createOwnedObject(size_t index)
{
	if(index < objects.size())
	{
		OwnedObjectInfo & obj = objects[index];
		std::string value = std::to_string(obj.count);
		auto data = std::make_shared<InfoBoxCustom>(value, "", AnimationPath::builtin("FLAGPORT"), obj.imageID, obj.hoverText);
		return std::make_shared<InfoBox>(Point(), InfoBox::POS_CORNER, InfoBox::SIZE_SMALL, data);
	}
	return std::shared_ptr<CIntObject>();
}

std::shared_ptr<CIntObject> CKingdomInterface::createMainTab(size_t index)
{
	size_t size = OVERVIEW_SIZE;
	switch(index)
	{
	case 0:
		return std::make_shared<CKingdHeroList>(size, [this](const CWindowWithArtifacts::CArtifactsOfHeroPtr & newHeroSet)
			{
				newHeroSet->clickPressedCallback = [this, newHeroSet](const CArtPlace & artPlace, const Point & cursorPosition)
				{
					clickPressedOnArtPlace(newHeroSet->getHero(), artPlace.slot, false, false, false, cursorPosition);
				};
				newHeroSet->showPopupCallback = [this, newHeroSet](CArtPlace & artPlace, const Point & cursorPosition)
				{
					showArtifactAssembling(*newHeroSet, artPlace, cursorPosition);
				};
				newHeroSet->gestureCallback = [this, newHeroSet](const CArtPlace & artPlace, const Point & cursorPosition)
				{
					showQuickBackpackWindow(newHeroSet->getHero(), artPlace.slot, cursorPosition);
				};
				addSet(newHeroSet);
			});
	case 1:
		return std::make_shared<CKingdTownList>(size);
	default:
		return std::shared_ptr<CIntObject>();
	}
}

void CKingdomInterface::generateMinesList(const std::vector<const CGObjectInstance *> & ownedObjects)
{
	ui32 footerPos = OVERVIEW_SIZE * 116;
	ResourceSet minesCount = ResourceSet();
	int totalIncome=0;

	for(const CGObjectInstance * object : ownedObjects)
	{
		//Mines
		if(object->ID == Obj::MINE || object->ID == Obj::ABANDONED_MINE)
		{
			const CGMine * mine = dynamic_cast<const CGMine *>(object);
			minesCount[mine->producedResource]++;
		}
	}

	for(auto & mapObject : ownedObjects)
		totalIncome += mapObject->asOwnable()->dailyIncome()[EGameResID::GOLD];

	//if player has some modded boosts we want to show that as well
	const auto * playerSettings = GAME->interface()->cb->getPlayerSettings(GAME->interface()->playerID);
	const auto & towns = GAME->interface()->cb->getTownsInfo(true);
	totalIncome += GAME->interface()->cb->getPlayerState(GAME->interface()->playerID)->valOfBonuses(BonusType::RESOURCES_CONSTANT_BOOST, BonusSubtypeID(GameResID(EGameResID::GOLD))) * playerSettings->handicap.percentIncome / 100;
	totalIncome += GAME->interface()->cb->getPlayerState(GAME->interface()->playerID)->valOfBonuses(BonusType::RESOURCES_TOWN_MULTIPLYING_BOOST, BonusSubtypeID(GameResID(EGameResID::GOLD))) * towns.size() * playerSettings->handicap.percentIncome / 100;

	for(int i=0; i<7; i++)
	{
		std::string value = std::to_string(minesCount[i]);
		auto data = std::make_shared<InfoBoxCustom>(value, "", AnimationPath::builtin("OVMINES"), i, LIBRARY->generaltexth->translate("core.minename", i));
		minesBox[i] = std::make_shared<InfoBox>(Point(20+i*80, 31+footerPos), InfoBox::POS_INSIDE, InfoBox::SIZE_SMALL, data);
		minesBox[i]->removeUsedEvents(LCLICK|SHOW_POPUP); //fixes #890 - mines boxes ignore clicks
	}
	incomeArea = std::make_shared<CHoverableArea>();
	incomeArea->pos = Rect(pos.x+580, pos.y+31+footerPos, 136, 68);
	incomeArea->hoverText = LIBRARY->generaltexth->allTexts[255];
	incomeAmount = std::make_shared<CLabel>(628, footerPos + 70, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, std::to_string(totalIncome));
}

void CKingdomInterface::generateButtons()
{
	ui32 footerPos = OVERVIEW_SIZE * 116;

	//Main control buttons
	btnHeroes = std::make_shared<CButton>(Point(748, 28+footerPos), AnimationPath::builtin("OVBUTN1.DEF"), CButton::tooltip(LIBRARY->generaltexth->overview[11], LIBRARY->generaltexth->overview[6]),
		std::bind(&CKingdomInterface::activateTab, this, 0), EShortcut::KINGDOM_HEROES_TAB);
	btnHeroes->block(true);

	btnTowns = std::make_shared<CButton>(Point(748, 64+footerPos), AnimationPath::builtin("OVBUTN6.DEF"), CButton::tooltip(LIBRARY->generaltexth->overview[12], LIBRARY->generaltexth->overview[7]),
		std::bind(&CKingdomInterface::activateTab, this, 1), EShortcut::KINGDOM_TOWNS_TAB);

	btnExit = std::make_shared<CButton>(Point(748,99+footerPos), AnimationPath::builtin("OVBUTN1.DEF"), CButton::tooltip(LIBRARY->generaltexth->allTexts[600]),
		std::bind(&CKingdomInterface::close, this), EShortcut::GLOBAL_RETURN);
	btnExit->setImageOrder(3, 4, 5, 6);

	//Object list control buttons
	dwellTop = std::make_shared<CButton>(Point(733, 4), AnimationPath::builtin("OVBUTN4.DEF"), CButton::tooltip(), [&](){ dwellingsList->moveToPos(0);});

	dwellBottom = std::make_shared<CButton>(Point(733, footerPos+2), AnimationPath::builtin("OVBUTN4.DEF"), CButton::tooltip(), [&](){ dwellingsList->moveToPos(-1); });
	dwellBottom->setImageOrder(2, 3, 4, 5);

	dwellUp = std::make_shared<CButton>(Point(733, 24), AnimationPath::builtin("OVBUTN4.DEF"), CButton::tooltip(), [&](){ dwellingsList->moveToPrev(); });
	dwellUp->setImageOrder(4, 5, 6, 7);

	dwellDown = std::make_shared<CButton>(Point(733, footerPos-18), AnimationPath::builtin("OVBUTN4.DEF"), CButton::tooltip(), [&](){ dwellingsList->moveToNext(); });
	dwellDown->setImageOrder(6, 7, 8, 9);
}

void CKingdomInterface::activateTab(size_t which)
{
	Settings s = settings.write["general"]["lastKindomInterface"];
	s->Integer() = which;

	btnHeroes->block(which == 0);
	btnTowns->block(which == 1);
	tabArea->setActive(which);
}

void CKingdomInterface::buildChanged()
{
	tabArea->reset();
}

void CKingdomInterface::townChanged(const CGTownInstance *town)
{
	if(auto townList = std::dynamic_pointer_cast<CKingdTownList>(tabArea->getItem()))
		townList->townChanged(town);
}

void CKingdomInterface::heroRemoved()
{
	tabArea->reset();
}

void CKingdomInterface::updateGarrisons()
{
	if(auto garrison = std::dynamic_pointer_cast<IGarrisonHolder>(tabArea->getItem()))
		garrison->updateGarrisons();
}

bool CKingdomInterface::holdsGarrison(const CArmedInstance * army)
{
	return army->getOwner() == GAME->interface()->playerID;
}

CKingdHeroList::CKingdHeroList(size_t maxSize, const CreateHeroItemFunctor & onCreateHeroItemCallback)
{
	OBJECT_CONSTRUCTION;
	title = std::make_shared<CPicture>(ImagePath::builtin("OVTITLE"),16,0);
	title->setPlayerColor(GAME->interface()->playerID);
	heroLabel = std::make_shared<CLabel>(150, 10, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->overview[0]);
	skillsLabel = std::make_shared<CLabel>(500, 10, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->overview[1]);

	ui32 townCount = GAME->interface()->cb->howManyHeroes(false);
	ui32 size = OVERVIEW_SIZE*116 + 19;
	heroes = std::make_shared<CListBox>([onCreateHeroItemCallback](size_t idx) -> std::shared_ptr<CIntObject>
		{
			auto heroesList = GAME->interface()->localState->getWanderingHeroes();
			if(idx < heroesList.size())
			{
				auto hero = std::make_shared<CHeroItem>(heroesList[idx]);
				onCreateHeroItemCallback(hero->heroArts);
				return hero;
			}
			else
			{
				return std::make_shared<CAnimImage>(AnimationPath::builtin("OVSLOT"), (idx - 2) % GameConstants::KINGDOM_WINDOW_HEROES_SLOTS);
			}
		}, Point(19,21), Point(0,116), maxSize, townCount, 0, 1, Rect(-19, -21, size, size));
}

void CKingdHeroList::updateGarrisons()
{
	for(std::shared_ptr<CIntObject> object : heroes->getItems())
	{
		if(IGarrisonHolder * garrison = dynamic_cast<IGarrisonHolder*>(object.get()))
			garrison->updateGarrisons();
	}
}

bool CKingdHeroList::holdsGarrison(const CArmedInstance * army)
{
	for(std::shared_ptr<CIntObject> object : heroes->getItems())
		if(IGarrisonHolder * garrison = dynamic_cast<IGarrisonHolder*>(object.get()))
			if (garrison->holdsGarrison(army))
				return true;
	return false;
}

CKingdTownList::CKingdTownList(size_t maxSize)
{
	OBJECT_CONSTRUCTION;
	title = std::make_shared<CPicture>(ImagePath::builtin("OVTITLE"), 16, 0);
	title->setPlayerColor(GAME->interface()->playerID);
	townLabel = std::make_shared<CLabel>(146, 10,FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->overview[3]);
	garrHeroLabel = std::make_shared<CLabel>(375, 10, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->overview[4]);
	visitHeroLabel = std::make_shared<CLabel>(608, 10, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->overview[5]);

	ui32 townCount = GAME->interface()->cb->howManyTowns();
	ui32 size = OVERVIEW_SIZE*116 + 19;
	towns = std::make_shared<CListBox>(std::bind(&CKingdTownList::createTownItem, this, _1),
		Point(19,21), Point(0,116), maxSize, townCount, 0, 1, Rect(-19, -21, size, size));
}

void CKingdTownList::townChanged(const CGTownInstance * town)
{
	for(std::shared_ptr<CIntObject> object : towns->getItems())
	{
		CTownItem * townItem = dynamic_cast<CTownItem *>(object.get());
		if(townItem && townItem->town == town)
			townItem->update();
	}
}

void CKingdTownList::updateGarrisons()
{
	for(std::shared_ptr<CIntObject> object : towns->getItems())
	{
		if(IGarrisonHolder * garrison = dynamic_cast<IGarrisonHolder*>(object.get()))
			garrison->updateGarrisons();
	}
}

bool CKingdTownList::holdsGarrison(const CArmedInstance * army)
{
	for(std::shared_ptr<CIntObject> object : towns->getItems())
		if(IGarrisonHolder * garrison = dynamic_cast<IGarrisonHolder*>(object.get()))
			if (garrison->holdsGarrison(army))
				return true;
	return false;
}

std::shared_ptr<CIntObject> CKingdTownList::createTownItem(size_t index)
{
	ui32 picCount = 4; // OVSLOT contains 4 images

	auto townsList = GAME->interface()->localState->getOwnedTowns();

	if(index < townsList.size())
		return std::make_shared<CTownItem>(townsList[index]);
	else
		return std::make_shared<CAnimImage>(AnimationPath::builtin("OVSLOT"), (index-2) % picCount );
}

CTownItem::CTownItem(const CGTownInstance * Town)
	: town(Town)
{
	OBJECT_CONSTRUCTION;
	background = std::make_shared<CAnimImage>(AnimationPath::builtin("OVSLOT"), 6);
	name = std::make_shared<CLabel>(74, 8, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, town->getNameTranslated());

	income = std::make_shared<CLabel>( 190, 60, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, std::to_string(town->dailyIncome()[EGameResID::GOLD]));
	hall = std::make_shared<CTownInfo>( 69, 31, town, true);
	fort = std::make_shared<CTownInfo>(111, 31, town, false);

	garr = std::make_shared<CGarrisonInt>(Point(313, 3), 4, Point(232,0), town->getUpperArmy(), town->getVisitingHero(), true, true, CGarrisonInt::ESlotsLayout::TWO_ROWS);
	heroes = std::make_shared<HeroSlots>(town, Point(244,6), Point(475,6), garr, false);

	size_t iconIndex = town->getTown()->clientInfo.icons[town->hasFort()][town->built >= GAME->interface()->cb->getSettings().getInteger(EGameSettings::TOWNS_BUILDINGS_PER_TURN_CAP)];

	picture = std::make_shared<CAnimImage>(AnimationPath::builtin("ITPT"), iconIndex, 0, 5, 6);
	openTown = std::make_shared<LRClickableAreaOpenTown>(Rect(5, 6, 58, 64), town);

	for(size_t i=0; i<town->creatures.size() && i<GameConstants::CREATURES_PER_TOWN; i++)
	{
		growth.push_back(std::make_shared<CCreaInfo>(Point(401+37*(int)i, 78), town, (int)i, true, true));
		available.push_back(std::make_shared<CCreaInfo>(Point(48+37*(int)i, 78), town, (int)i, true, false));
	}

	fastTownHall = std::make_shared<CButton>(Point(69, 31), AnimationPath::builtin("castleInterfaceQuickAccess"), CButton::tooltip(), [this]() { std::make_shared<CCastleBuildings>(town)->enterTownHall(); });
	fastTownHall->setOverlay(std::make_shared<CAnimImage>(AnimationPath::builtin("ITMTL"), town->hallLevel()));

	int imageIndex = town->fortLevel() == CGTownInstance::EFortLevel::NONE ? 3 : town->fortLevel() - 1;
	fastArmyPurchase = std::make_shared<CButton>(Point(111, 31), AnimationPath::builtin("castleInterfaceQuickAccess"), CButton::tooltip(), [this]() { std::make_shared<CCastleBuildings>(town)->enterToTheQuickRecruitmentWindow(); });
	fastArmyPurchase->setOverlay(std::make_shared<CAnimImage>(AnimationPath::builtin("itmcl"), imageIndex));

	fastTavern = std::make_shared<LRClickableArea>(Rect(5, 6, 58, 64), [&]()
	{
		if(town->hasBuilt(BuildingID::TAVERN))
			GAME->interface()->showTavernWindow(town, nullptr, QueryID::NONE);
	}, [&]{
		if(!town->getTown()->faction->getDescriptionTranslated().empty())
			CRClickPopup::createAndPush(town->getFaction()->getDescriptionTranslated());
	});
	fastMarket = std::make_shared<LRClickableArea>(Rect(153, 6, 65, 64), []()
	{
		std::vector<const CGTownInstance*> towns = GAME->interface()->cb->getTownsInfo(true);
		for(auto & town : towns)
		{
			if(town->hasBuilt(BuildingID::MARKETPLACE))
			{
				ENGINE->windows().createAndPushWindow<CMarketWindow>(town, nullptr, nullptr, EMarketMode::RESOURCE_RESOURCE);
				return;
			}
		}
		GAME->interface()->showInfoDialog(LIBRARY->generaltexth->translate("vcmi.adventureMap.noTownWithMarket"));
	});
	fastTown = std::make_shared<LRClickableArea>(Rect(67, 6, 165, 20), [&]()
	{
		ENGINE->windows().createAndPushWindow<CCastleInterface>(town);
	});

	labelCreatureGrowth = std::make_shared<CMultiLineLabel>(Rect(4, 76, 50, 35), EFonts::FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, LIBRARY->generaltexth->translate("core.genrltxt.265"));
	labelCreatureAvailable = std::make_shared<CMultiLineLabel>(Rect(349, 76, 57, 35), EFonts::FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, LIBRARY->generaltexth->translate("core.genrltxt.266"));
}

void CTownItem::updateGarrisons()
{
	garr->selectSlot(nullptr);
	garr->setArmy(town->getUpperArmy(), EGarrisonType::UPPER);
	garr->setArmy(town->getVisitingHero(), EGarrisonType::LOWER);
	garr->recreateSlots();
}

bool CTownItem::holdsGarrison(const CArmedInstance * army)
{
	return army == town || army == town->getUpperArmy() || army == town->getVisitingHero();
}

void CTownItem::update()
{
	std::string incomeVal = std::to_string(town->dailyIncome()[EGameResID::GOLD]);
	if (incomeVal != income->getText())
		income->setText(incomeVal);

	heroes->update();

	for (size_t i=0; i<std::min(static_cast<int>(town->creatures.size()), GameConstants::CREATURES_PER_TOWN); i++)
	{
		growth[i]->update();
		available[i]->update();
	}
}

class ArtSlotsTab : public CIntObject
{
public:
	std::shared_ptr<CAnimImage> background;
	std::vector<std::shared_ptr<CArtPlace>> arts;

	ArtSlotsTab(CIntObject * parent)
	{
		OBJECT_CONSTRUCTION_TARGETED(parent);
		background = std::make_shared<CAnimImage>(AnimationPath::builtin("OVSLOT"), 4);
		pos = background->pos;
		for(int i=0; i<9; i++)
			arts.push_back(std::make_shared<CArtPlace>(Point(269+i*48, 66)));
	}
};

class BackpackTab : public CIntObject
{
public:
	std::shared_ptr<CAnimImage> background;
	std::vector<std::shared_ptr<CArtPlace>> arts;
	std::shared_ptr<CButton> btnLeft;
	std::shared_ptr<CButton> btnRight;

	BackpackTab(CIntObject * parent)
	{
		OBJECT_CONSTRUCTION_TARGETED(parent);
		background = std::make_shared<CAnimImage>(AnimationPath::builtin("OVSLOT"), 5);
		pos = background->pos;
		btnLeft = std::make_shared<CButton>(Point(269, 66), AnimationPath::builtin("HSBTNS3"), CButton::tooltip(), 0);
		btnRight = std::make_shared<CButton>(Point(675, 66), AnimationPath::builtin("HSBTNS5"), CButton::tooltip(), 0);
		for(int i=0; i<8; i++)
			arts.push_back(std::make_shared<CArtPlace>(Point(294+i*48, 66)));
	}
};

CHeroItem::CHeroItem(const CGHeroInstance * Hero)
	: hero(Hero)
{
	OBJECT_CONSTRUCTION;

	artTabs.resize(3);
	auto arts1 = std::make_shared<ArtSlotsTab>(this);
	auto arts2 = std::make_shared<ArtSlotsTab>(this);
	auto backpack = std::make_shared<BackpackTab>(this);
	artTabs[0] = arts1;
	artTabs[1] = arts2;
	artTabs[2] = backpack;
	arts1->recActions = SHARE_POS;
	arts2->recActions = SHARE_POS;
	backpack->recActions = SHARE_POS;

	name = std::make_shared<CLabel>(75, 7, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, hero->getNameTranslated());

	//layout is not trivial: MACH4 - catapult - excluded, MISC[x] rearranged
	assert(arts1->arts.size() == 9);
	assert(arts2->arts.size() == 9);

	CArtifactsOfHeroMain::ArtPlaceMap arts =
	{
		{ArtifactPosition::HEAD, arts1->arts[0]},
		{ArtifactPosition::SHOULDERS,arts1->arts[1]},
		{ArtifactPosition::NECK,arts1->arts[2]},
		{ArtifactPosition::RIGHT_HAND,arts1->arts[3]},
		{ArtifactPosition::LEFT_HAND,arts1->arts[4]},
		{ArtifactPosition::TORSO, arts1->arts[5]},
		{ArtifactPosition::RIGHT_RING,arts1->arts[6]},
		{ArtifactPosition::LEFT_RING, arts1->arts[7]},
		{ArtifactPosition::FEET, arts1->arts[8]},

		{ArtifactPosition::MISC1, arts2->arts[0]},
		{ArtifactPosition::MISC2, arts2->arts[1]},
		{ArtifactPosition::MISC3, arts2->arts[2]},
		{ArtifactPosition::MISC4, arts2->arts[3]},
		{ArtifactPosition::MISC5, arts2->arts[4]},
		{ArtifactPosition::MACH1, arts2->arts[5]},
		{ArtifactPosition::MACH2, arts2->arts[6]},
		{ArtifactPosition::MACH3, arts2->arts[7]},
		{ArtifactPosition::SPELLBOOK, arts2->arts[8]}
	};


	heroArts = std::make_shared<CArtifactsOfHeroKingdom>(arts, backpack->arts, backpack->btnLeft, backpack->btnRight);
	heroArts->setHero(hero);

	artsTabs = std::make_shared<CTabbedInt>(std::bind(&CHeroItem::onTabSelected, this, _1));

	artButtons = std::make_shared<CToggleGroup>(0);
	for(size_t it = 0; it<3; it++)
	{
		int stringID[3] = {259, 261, 262};

		std::string hover = LIBRARY->generaltexth->overview[13+it];
		std::string overlay = LIBRARY->generaltexth->overview[8+it];

		auto button = std::make_shared<CToggleButton>(Point(364+(int)it*112, 46), AnimationPath::builtin("OVBUTN3"), CButton::tooltip(hover, overlay), 0);
		button->setTextOverlay(LIBRARY->generaltexth->allTexts[stringID[it]], FONT_SMALL, Colors::YELLOW);
		artButtons->addToggle((int)it, button);
	}
	artButtons->addCallback(std::bind(&CTabbedInt::setActive, artsTabs, _1));
	artButtons->addCallback(std::bind(&CHeroItem::onArtChange, this, _1));
	artButtons->setSelected(0);

	garr = std::make_shared<CGarrisonInt>(Point(6, 78), 4, Point(), hero, nullptr, true, true);

	portrait = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), hero->getIconIndex(), 0, 5, 6);
	heroArea = std::make_shared<CHeroArea>(5, 6, hero);

	name = std::make_shared<CLabel>(73, 7, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, hero->getNameTranslated());
	artsText = std::make_shared<CLabel>(320, 55, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->overview[2]);

	for(size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
	{
		auto data = std::make_shared<InfoBoxHeroData>(IInfoBoxData::HERO_PRIMARY_SKILL, hero, (int)i);
		heroInfo.push_back(std::make_shared<InfoBox>(Point(78+(int)i*36, 26), InfoBox::POS_DOWN, InfoBox::SIZE_SMALL, data));
	}

	int slots = 8;
	bool isMoreSkillsThanSlots = hero->secSkills.size() > slots;
	for(size_t i=0; i<slots; i++)
	{
		if(isMoreSkillsThanSlots && i == slots - 1)
		{
			Rect r(Point(410+(int)i*36, 5), Point(34, 28));
			heroInfoFull = std::make_shared<CMultiLineLabel>(r, EFonts::FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, "...");
			heroInfoFullArea = std::make_shared<LRClickableAreaWText>(r, LIBRARY->generaltexth->translate("vcmi.kingdomOverview.secSkillOverflow.hover"), LIBRARY->generaltexth->translate("vcmi.kingdomOverview.secSkillOverflow.help"));
			continue;
		}
		auto data = std::make_shared<InfoBoxHeroData>(IInfoBoxData::HERO_SECONDARY_SKILL, hero, (int)i);
		heroInfo.push_back(std::make_shared<InfoBox>(Point(410+(int)i*36, 5), InfoBox::POS_NONE, InfoBox::SIZE_SMALL, data));
	}

	{
		auto data = std::make_shared<InfoBoxHeroData>(IInfoBoxData::HERO_SPECIAL, hero);
		heroInfo.push_back(std::make_shared<InfoBox>(Point(375, 5), InfoBox::POS_NONE, InfoBox::SIZE_SMALL, data));

		data = std::make_shared<InfoBoxHeroData>(IInfoBoxData::HERO_EXPERIENCE, hero);
		heroInfo.push_back(std::make_shared<InfoBox>(Point(330, 5), InfoBox::POS_INSIDE, InfoBox::SIZE_SMALL, data));

		data = std::make_shared<InfoBoxHeroData>(IInfoBoxData::HERO_MANA, hero);
		heroInfo.push_back(std::make_shared<InfoBox>(Point(280, 5), InfoBox::POS_INSIDE, InfoBox::SIZE_SMALL, data));
	}

	morale = std::make_shared<MoraleLuckBox>(true, Rect(225, 53, 30, 22), true);
	luck = std::make_shared<MoraleLuckBox>(false, Rect(225, 28, 30, 22), true);

	morale->set(hero);
	luck->set(hero);
}


void CHeroItem::updateGarrisons()
{
	garr->recreateSlots();
}

bool CHeroItem::holdsGarrison(const CArmedInstance * army)
{
	return hero == army;
}

std::shared_ptr<CIntObject> CHeroItem::onTabSelected(size_t index)
{
	return artTabs.at(index);
}

void CHeroItem::onArtChange(int tabIndex)
{
	//redraw item after background change
	if(isActive())
		redraw();
}
