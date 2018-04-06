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

#include "CAdvmapInterface.h"
#include "CCastleInterface.h"

#include "../CGameInfo.h"
#include "../CMT.h"
#include "../CPlayerInterface.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/MiscWidgets.h"
#include "../windows/InfoWindows.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CModHandler.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"

InfoBox::InfoBox(Point position, InfoPos Pos, InfoSize Size, std::shared_ptr<IInfoBoxData> Data):
	size(Size),
	infoPos(Pos),
	data(Data),
	value(nullptr),
	name(nullptr)
{
	assert(data);
	addUsedEvents(LCLICK | RCLICK);
	EFonts font = (size < SIZE_MEDIUM)? FONT_SMALL: FONT_MEDIUM;

	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	pos+=position;

	image = std::make_shared<CAnimImage>(data->getImageName(size), data->getImageIndex());
	pos = image->pos;

	switch(infoPos)
	{
	case POS_CORNER:
		value = std::make_shared<CLabel>(pos.w, pos.h, font, BOTTOMRIGHT, Colors::WHITE, data->getValueText());
		break;
	case POS_INSIDE:
		value = std::make_shared<CLabel>(pos.w/2, pos.h-6, font, CENTER, Colors::WHITE, data->getValueText());
		break;
	case POS_UP_DOWN:
		name = std::make_shared<CLabel>(pos.w/2, -12, font, CENTER, Colors::WHITE, data->getNameText());
		FALLTHROUGH;
	case POS_DOWN:
		value = std::make_shared<CLabel>(pos.w/2, pos.h+8, font, CENTER, Colors::WHITE, data->getValueText());
		break;
	case POS_RIGHT:
		name = std::make_shared<CLabel>(pos.w+6, 6, font, TOPLEFT, Colors::WHITE, data->getNameText());
		value = std::make_shared<CLabel>(pos.w+6, pos.h-16, font, TOPLEFT, Colors::WHITE, data->getValueText());
		break;
	}

	if(name)
		pos = pos | name->pos;
	if(value)
		pos = pos | value->pos;

	hover = std::make_shared<CHoverableArea>();
	hover->hoverText = data->getHoverText();
	hover->pos = pos;
}

InfoBox::~InfoBox() = default;

void InfoBox::clickRight(tribool down, bool previousState)
{
	if (down)
	{
		std::shared_ptr<CComponent> comp;
		std::string text;
		data->prepareMessage(text, comp);
		if (comp)
			CRClickPopup::createAndPush(text, CInfoWindow::TCompsInfo(1, comp));
		else if (!text.empty())
			adventureInt->handleRightClick(text, down);
	}
}

void InfoBox::clickLeft(tribool down, bool previousState)
{
	if((!down) && previousState)
	{
		std::shared_ptr<CComponent> comp;
		std::string text;
		data->prepareMessage(text, comp);

		if(comp)
			LOCPLINT->showInfoDialog(text, CInfoWindow::TCompsInfo(1, comp));
	}
}

//TODO?
/*
void InfoBox::update()
{

}
*/

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
	case HERO_EXPERIENCE:
	case HERO_PRIMARY_SKILL:
		return boost::lexical_cast<std::string>(getValue());
	case HERO_SPECIAL:
		return CGI->generaltexth->jktexts[5];
	case HERO_SECONDARY_SKILL:
		{
			si64 value = getValue();
			if (value)
				return CGI->generaltexth->levels[value];
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
		return CGI->generaltexth->primarySkillNames[getSubID()];
	case HERO_MANA:
		return CGI->generaltexth->allTexts[387];
	case HERO_EXPERIENCE:
		return CGI->generaltexth->jktexts[6];
	case HERO_SPECIAL:
		return CGI->heroh->heroes[getSubID()]->specName;
	case HERO_SECONDARY_SKILL:
		if (getValue())
			return CGI->skillh->skillName(getSubID());
		else
			return "";
	default:
		logGlobal->error("Invalid InfoBox info type");
	}
	return "";
}

std::string InfoBoxAbstractHeroData::getImageName(InfoBox::InfoSize size)
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
				return "PSKIL32";
			case HERO_SPECIAL:
				return "UN32";
			case HERO_SECONDARY_SKILL:
				return "SECSK32";
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
				return "PSKIL42";
			case HERO_SPECIAL:
				return "UN44";
			case HERO_SECONDARY_SKILL:
				return "SECSKILL";
			default:
				assert(0);
			}
		}
	default:
		assert(0);
	}
	return "";
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
		return VLC->heroh->heroes[getSubID()]->imageIndex;
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
		text = CGI->heroh->heroes[getSubID()]->specDescr;
		break;
	case HERO_PRIMARY_SKILL:
		text = CGI->generaltexth->arraytxt[2+getSubID()];
		comp = std::make_shared<CComponent>(CComponent::primskill, getSubID(), getValue());
		break;
	case HERO_MANA:
		text = CGI->generaltexth->allTexts[149];
		break;
	case HERO_EXPERIENCE:
		text = CGI->generaltexth->allTexts[241];
		break;
	case HERO_SECONDARY_SKILL:
		{
			si64 value = getValue();
			int  subID = getSubID();
			if(value)
			{
				text = CGI->skillh->skillInfo(subID, value);
				comp = std::make_shared<CComponent>(CComponent::secskill, subID, value);
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
			return hero->secSkills[index].first;
		else
			return 0;
	case HERO_SPECIAL:
		return hero->type->ID.getNum();
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
	switch(type)
	{
	case HERO_PRIMARY_SKILL:
		return hero->getPrimSkillLevel(static_cast<PrimarySkill::PrimarySkill>(index));
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
		return boost::str(boost::format(CGI->generaltexth->heroscrn[1]) % CGI->generaltexth->primarySkillNames[index]);
	case HERO_MANA:
		return CGI->generaltexth->heroscrn[22];
	case HERO_EXPERIENCE:
		return CGI->generaltexth->heroscrn[9];
	case HERO_SPECIAL:
		return CGI->generaltexth->heroscrn[27];
	case HERO_SECONDARY_SKILL:
		if (hero->secSkills.size() > index)
		{
			std::string level = CGI->generaltexth->levels[hero->secSkills[index].second-1];
			std::string skill = CGI->skillh->skillName(hero->secSkills[index].first);
			return boost::str(boost::format(CGI->generaltexth->heroscrn[21]) % level % skill);
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
			return boost::lexical_cast<std::string>(hero->mana) + '/' +
				boost::lexical_cast<std::string>(hero->manaLimit());
		case HERO_EXPERIENCE:
			return boost::lexical_cast<std::string>(hero->exp);
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
		text = CGI->generaltexth->allTexts[205];
		boost::replace_first(text, "%s", boost::lexical_cast<std::string>(hero->name));
		boost::replace_first(text, "%d", boost::lexical_cast<std::string>(hero->mana));
		boost::replace_first(text, "%d", boost::lexical_cast<std::string>(hero->manaLimit()));
		break;
	case HERO_EXPERIENCE:
		text = CGI->generaltexth->allTexts[2];
		boost::replace_first(text, "%d", boost::lexical_cast<std::string>(hero->level));
		boost::replace_first(text, "%d", boost::lexical_cast<std::string>(CGI->heroh->reqExp(hero->level+1)));
		boost::replace_first(text, "%d", boost::lexical_cast<std::string>(hero->exp));
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

InfoBoxCustom::InfoBoxCustom(std::string ValueText, std::string NameText, std::string ImageName, size_t ImageIndex, std::string HoverText):
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

std::string InfoBoxCustom::getImageName(InfoBox::InfoSize size)
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
	: CWindowObject(PLAYER_COLORED | BORDERED, conf.go()->ac.overviewBg)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	ui32 footerPos = conf.go()->ac.overviewSize * 116;

	tabArea = std::make_shared<CTabbedInt>(std::bind(&CKingdomInterface::createMainTab, this, _1), Point(4,4));

	std::vector<const CGObjectInstance * > ownedObjects = LOCPLINT->cb->getMyObjects();
	generateObjectsList(ownedObjects);
	generateMinesList(ownedObjects);
	generateButtons();

	statusbar = std::make_shared<CGStatusBar>(std::make_shared<CPicture>("KSTATBAR", 10,pos.h - 45));
	resdatabar = std::make_shared<CResDataBar>("KRESBAR", 3, 111+footerPos, 32, 2, 76, 76);
}

void CKingdomInterface::generateObjectsList(const std::vector<const CGObjectInstance * > &ownedObjects)
{
	ui32 footerPos = conf.go()->ac.overviewSize * 116;
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
		std::string value = boost::lexical_cast<std::string>(obj.count);
		auto data = std::make_shared<InfoBoxCustom>(value, "", "FLAGPORT", obj.imageID, obj.hoverText);
		return std::make_shared<InfoBox>(Point(), InfoBox::POS_CORNER, InfoBox::SIZE_SMALL, data);
	}
	return std::shared_ptr<CIntObject>();
}

std::shared_ptr<CIntObject> CKingdomInterface::createMainTab(size_t index)
{
	size_t size = conf.go()->ac.overviewSize;
	switch(index)
	{
	case 0:
		return std::make_shared<CKingdHeroList>(size);
	case 1:
		return std::make_shared<CKingdTownList>(size);
	default:
		return std::shared_ptr<CIntObject>();
	}
}

void CKingdomInterface::generateMinesList(const std::vector<const CGObjectInstance *> & ownedObjects)
{
	ui32 footerPos = conf.go()->ac.overviewSize * 116;
	std::vector<int> minesCount(GameConstants::RESOURCE_QUANTITY, 0);
	int totalIncome=0;

	for(const CGObjectInstance * object : ownedObjects)
	{
		//Mines
		if(object->ID == Obj::MINE || object->ID == Obj::ABANDONED_MINE)
		{
			const CGMine * mine = dynamic_cast<const CGMine *>(object);
			assert(mine);
			minesCount[mine->producedResource]++;

			if (mine->producedResource == Res::GOLD)
				totalIncome += mine->producedQuantity;
		}
	}

	//Heroes can produce gold as well - skill, specialty or arts
	std::vector<const CGHeroInstance*> heroes = LOCPLINT->cb->getHeroesInfo(true);
	for(auto & heroe : heroes)
	{
		totalIncome += heroe->valOfBonuses(Selector::typeSubtype(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::ESTATES));
		totalIncome += heroe->valOfBonuses(Selector::typeSubtype(Bonus::GENERATE_RESOURCE, Res::GOLD));
	}

	//Add town income of all towns
	std::vector<const CGTownInstance*> towns = LOCPLINT->cb->getTownsInfo(true);
	for(auto & town : towns)
	{
		totalIncome += town->dailyIncome()[Res::GOLD];
	}
	for(int i=0; i<7; i++)
	{
		std::string value = boost::lexical_cast<std::string>(minesCount[i]);
		auto data = std::make_shared<InfoBoxCustom>(value, "", "OVMINES", i, CGI->generaltexth->mines[i].first);
		minesBox[i] = std::make_shared<InfoBox>(Point(20+i*80, 31+footerPos), InfoBox::POS_INSIDE, InfoBox::SIZE_SMALL, data);
		minesBox[i]->removeUsedEvents(LCLICK|RCLICK); //fixes #890 - mines boxes ignore clicks
	}
	incomeArea = std::make_shared<CHoverableArea>();
	incomeArea->pos = Rect(pos.x+580, pos.y+31+footerPos, 136, 68);
	incomeArea->hoverText = CGI->generaltexth->allTexts[255];
	incomeAmount = std::make_shared<CLabel>(628, footerPos + 70, FONT_SMALL, TOPLEFT, Colors::WHITE, boost::lexical_cast<std::string>(totalIncome));
}

void CKingdomInterface::generateButtons()
{
	ui32 footerPos = conf.go()->ac.overviewSize * 116;

	//Main control buttons
	btnHeroes = std::make_shared<CButton>(Point(748, 28+footerPos), "OVBUTN1.DEF", CButton::tooltip(CGI->generaltexth->overview[11], CGI->generaltexth->overview[6]),
		std::bind(&CKingdomInterface::activateTab, this, 0), SDLK_h);
	btnHeroes->block(true);

	btnTowns = std::make_shared<CButton>(Point(748, 64+footerPos), "OVBUTN6.DEF", CButton::tooltip(CGI->generaltexth->overview[12], CGI->generaltexth->overview[7]),
		std::bind(&CKingdomInterface::activateTab, this, 1), SDLK_t);

	btnExit = std::make_shared<CButton>(Point(748,99+footerPos), "OVBUTN1.DEF", CButton::tooltip(CGI->generaltexth->allTexts[600]),
		std::bind(&CKingdomInterface::close, this), SDLK_RETURN);
	btnExit->assignedKeys.insert(SDLK_ESCAPE);
	btnExit->setImageOrder(3, 4, 5, 6);

	//Object list control buttons
	dwellTop = std::make_shared<CButton>(Point(733, 4), "OVBUTN4.DEF", CButton::tooltip(), [&](){ dwellingsList->moveToPos(0);});

	dwellBottom = std::make_shared<CButton>(Point(733, footerPos+2), "OVBUTN4.DEF", CButton::tooltip(), [&](){ dwellingsList->moveToPos(-1); });
	dwellBottom->setImageOrder(2, 3, 4, 5);

	dwellUp = std::make_shared<CButton>(Point(733, 24), "OVBUTN4.DEF", CButton::tooltip(), [&](){ dwellingsList->moveToPrev(); });
	dwellUp->setImageOrder(4, 5, 6, 7);

	dwellDown = std::make_shared<CButton>(Point(733, footerPos-18), "OVBUTN4.DEF", CButton::tooltip(), [&](){ dwellingsList->moveToNext(); });
	dwellDown->setImageOrder(6, 7, 8, 9);
}

void CKingdomInterface::activateTab(size_t which)
{
	btnHeroes->block(which == 0);
	btnTowns->block(which == 1);
	tabArea->setActive(which);
}

void CKingdomInterface::townChanged(const CGTownInstance *town)
{
	if(auto townList = std::dynamic_pointer_cast<CKingdTownList>(tabArea->getItem()))
		townList->townChanged(town);
}

void CKingdomInterface::updateGarrisons()
{
	if(auto garrison = std::dynamic_pointer_cast<CGarrisonHolder>(tabArea->getItem()))
		garrison->updateGarrisons();
}

void CKingdomInterface::artifactAssembled(const ArtifactLocation& artLoc)
{
	if(auto arts = std::dynamic_pointer_cast<CArtifactHolder>(tabArea->getItem()))
		arts->artifactAssembled(artLoc);
}

void CKingdomInterface::artifactDisassembled(const ArtifactLocation& artLoc)
{
	if(auto arts = std::dynamic_pointer_cast<CArtifactHolder>(tabArea->getItem()))
		arts->artifactDisassembled(artLoc);
}

void CKingdomInterface::artifactMoved(const ArtifactLocation& artLoc, const ArtifactLocation& destLoc)
{
	if(auto arts = std::dynamic_pointer_cast<CArtifactHolder>(tabArea->getItem()))
		arts->artifactMoved(artLoc, destLoc);
}

void CKingdomInterface::artifactRemoved(const ArtifactLocation& artLoc)
{
	if(auto arts = std::dynamic_pointer_cast<CArtifactHolder>(tabArea->getItem()))
		arts->artifactRemoved(artLoc);
}

CKingdHeroList::CKingdHeroList(size_t maxSize)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	title = std::make_shared<CPicture>("OVTITLE",16,0);
	title->colorize(LOCPLINT->playerID);
	heroLabel = std::make_shared<CLabel>(150, 10, FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->overview[0]);
	skillsLabel = std::make_shared<CLabel>(500, 10, FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->overview[1]);

	ui32 townCount = LOCPLINT->cb->howManyHeroes(false);
	ui32 size = conf.go()->ac.overviewSize*116 + 19;
	heroes = std::make_shared<CListBox>(std::bind(&CKingdHeroList::createHeroItem, this, _1),
		Point(19,21), Point(0,116), maxSize, townCount, 0, 1, Rect(-19, -21, size, size));
}

void CKingdHeroList::updateGarrisons()
{
	for(std::shared_ptr<CIntObject> object : heroes->getItems())
	{
		if(CGarrisonHolder * garrison = dynamic_cast<CGarrisonHolder*>(object.get()))
			garrison->updateGarrisons();
	}
}

std::shared_ptr<CIntObject> CKingdHeroList::createHeroItem(size_t index)
{
	ui32 picCount = conf.go()->ac.overviewPics;
	size_t heroesCount = LOCPLINT->cb->howManyHeroes(false);

	if(index < heroesCount)
	{
		auto hero = std::make_shared<CHeroItem>(LOCPLINT->cb->getHeroBySerial(index, false));
		addSet(hero->heroArts);
		return hero;
	}
	else
	{
		return std::make_shared<CAnimImage>("OVSLOT", (index-2) % picCount );
	}
}

CKingdTownList::CKingdTownList(size_t maxSize)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	title = std::make_shared<CPicture>("OVTITLE", 16, 0);
	title->colorize(LOCPLINT->playerID);
	townLabel = std::make_shared<CLabel>(146, 10,FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->overview[3]);
	garrHeroLabel = std::make_shared<CLabel>(375, 10, FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->overview[4]);
	visitHeroLabel = std::make_shared<CLabel>(608, 10, FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->overview[5]);

	ui32 townCount = LOCPLINT->cb->howManyTowns();
	ui32 size = conf.go()->ac.overviewSize*116 + 19;
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
		if(CGarrisonHolder * garrison = dynamic_cast<CGarrisonHolder*>(object.get()))
			garrison->updateGarrisons();
	}
}

std::shared_ptr<CIntObject> CKingdTownList::createTownItem(size_t index)
{
	ui32 picCount = conf.go()->ac.overviewPics;
	size_t townsCount = LOCPLINT->cb->howManyTowns();

	if(index < townsCount)
		return std::make_shared<CTownItem>(LOCPLINT->cb->getTownBySerial(index));
	else
		return std::make_shared<CAnimImage>("OVSLOT", (index-2) % picCount );
}

CTownItem::CTownItem(const CGTownInstance * Town)
	: town(Town)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	background = std::make_shared<CAnimImage>("OVSLOT", 6);
	name = std::make_shared<CLabel>(74, 8, FONT_SMALL, TOPLEFT, Colors::WHITE, town->name);

	income = std::make_shared<CLabel>( 190, 60, FONT_SMALL, CENTER, Colors::WHITE, boost::lexical_cast<std::string>(town->dailyIncome()[Res::GOLD]));
	hall = std::make_shared<CTownInfo>( 69, 31, town, true);
	fort = std::make_shared<CTownInfo>(111, 31, town, false);

	garr = std::make_shared<CGarrisonInt>(313, 3, 4, Point(232,0), town->getUpperArmy(), town->visitingHero, true, true, true);
	heroes = std::make_shared<HeroSlots>(town, Point(244,6), Point(475,6), garr, false);

	size_t iconIndex = town->town->clientInfo.icons[town->hasFort()][town->builded >= CGI->modh->settings.MAX_BUILDING_PER_TURN];

	picture = std::make_shared<CAnimImage>("ITPT", iconIndex, 0, 5, 6);
	openTown = std::make_shared<LRClickableAreaOpenTown>(Rect(5, 6, 58, 64), town);

	for(size_t i=0; i<town->creatures.size(); i++)
	{
		growth.push_back(std::make_shared<CCreaInfo>(Point(401+37*i, 78), town, i, true, true));
		available.push_back(std::make_shared<CCreaInfo>(Point(48+37*i, 78), town, i, true, false));
	}
}

void CTownItem::updateGarrisons()
{
	garr->selectSlot(nullptr);
	garr->setArmy(town->getUpperArmy(), 0);
	garr->setArmy(town->visitingHero, 1);
	garr->recreateSlots();
}

void CTownItem::update()
{
	std::string incomeVal = boost::lexical_cast<std::string>(town->dailyIncome()[Res::GOLD]);
	if (incomeVal != income->text)
		income->setText(incomeVal);

	heroes->update();

	for (size_t i=0; i<town->creatures.size(); i++)
	{
		growth[i]->update();
		available[i]->update();
	}
}

class ArtSlotsTab : public CIntObject
{
public:
	std::shared_ptr<CAnimImage> background;
	std::vector<std::shared_ptr<CHeroArtPlace>> arts;

	ArtSlotsTab()
	{
		OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
		background = std::make_shared<CAnimImage>("OVSLOT", 4);
		pos = background->pos;
		for(size_t i=0; i<9; i++)
			arts.push_back(std::make_shared<CHeroArtPlace>(Point(270+i*48, 65)));
	}
};

class BackpackTab : public CIntObject
{
public:
	std::shared_ptr<CAnimImage> background;
	std::vector<std::shared_ptr<CHeroArtPlace>> arts;
	std::shared_ptr<CButton> btnLeft;
	std::shared_ptr<CButton> btnRight;

	BackpackTab()
	{
		OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
		background = std::make_shared<CAnimImage>("OVSLOT", 5);
		pos = background->pos;
		btnLeft = std::make_shared<CButton>(Point(269, 66), "HSBTNS3", CButton::tooltip(), 0);
		btnRight = std::make_shared<CButton>(Point(675, 66), "HSBTNS5", CButton::tooltip(), 0);
		for(size_t i=0; i<8; i++)
			arts.push_back(std::make_shared<CHeroArtPlace>(Point(295+i*48, 65)));
	}
};

CHeroItem::CHeroItem(const CGHeroInstance * Hero)
	: hero(Hero)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	artTabs.resize(3);
	auto arts1 = std::make_shared<ArtSlotsTab>();
	auto arts2 = std::make_shared<ArtSlotsTab>();
	auto backpack = std::make_shared<BackpackTab>();
	artTabs[0] = arts1;
	artTabs[1] = arts2;
	artTabs[2] = backpack;
	arts1->recActions = SHARE_POS;
	arts2->recActions = SHARE_POS;
	backpack->recActions = SHARE_POS;

	name = std::make_shared<CLabel>(75, 7, FONT_SMALL, TOPLEFT, Colors::WHITE, hero->name);

	//layout is not trivial: MACH4 - catapult - excluded, MISC[x] rearranged
	assert(arts1->arts.size() == 9);
	assert(arts2->arts.size() == 9);

	CArtifactsOfHero::ArtPlaceMap arts =
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


	heroArts = std::make_shared<CArtifactsOfHero>(arts, backpack->arts, backpack->btnLeft, backpack->btnRight, true);
	heroArts->setHero(hero);

	artsTabs = std::make_shared<CTabbedInt>(std::bind(&CHeroItem::onTabSelected, this, _1));

	artButtons = std::make_shared<CToggleGroup>(0);
	for(size_t it = 0; it<3; it++)
	{
		int stringID[3] = {259, 261, 262};

		std::string hover = CGI->generaltexth->overview[13+it];
		std::string overlay = CGI->generaltexth->overview[8+it];

		auto button = std::make_shared<CToggleButton>(Point(364+it*112, 46), "OVBUTN3", CButton::tooltip(hover, overlay), 0);
		button->addTextOverlay(CGI->generaltexth->allTexts[stringID[it]], FONT_SMALL, Colors::YELLOW);
		artButtons->addToggle(it, button);
	}
	artButtons->addCallback(std::bind(&CTabbedInt::setActive, artsTabs, _1));
	artButtons->addCallback(std::bind(&CHeroItem::onArtChange, this, _1));
	artButtons->setSelected(0);

	garr = std::make_shared<CGarrisonInt>(6, 78, 4, Point(), hero, nullptr, true, true);

	portrait = std::make_shared<CAnimImage>("PortraitsLarge", hero->portrait, 0, 5, 6);
	heroArea = std::make_shared<CHeroArea>(5, 6, hero);

	name = std::make_shared<CLabel>(73, 7, FONT_SMALL, TOPLEFT, Colors::WHITE, hero->name);
	artsText = std::make_shared<CLabel>(320, 55, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->overview[2]);

	for(size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
	{
		auto data = std::make_shared<InfoBoxHeroData>(IInfoBoxData::HERO_PRIMARY_SKILL, hero, i);
		heroInfo.push_back(std::make_shared<InfoBox>(Point(78+i*36, 26), InfoBox::POS_DOWN, InfoBox::SIZE_SMALL, data));
	}

	for(size_t i=0; i<GameConstants::SKILL_PER_HERO; i++)
	{
		auto data = std::make_shared<InfoBoxHeroData>(IInfoBoxData::HERO_SECONDARY_SKILL, hero, i);
		heroInfo.push_back(std::make_shared<InfoBox>(Point(410+i*36, 5), InfoBox::POS_NONE, InfoBox::SIZE_SMALL, data));
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

std::shared_ptr<CIntObject> CHeroItem::onTabSelected(size_t index)
{
	return artTabs.at(index);
}

void CHeroItem::onArtChange(int tabIndex)
{
	//redraw item after background change
	if(active)
		redraw();
}
