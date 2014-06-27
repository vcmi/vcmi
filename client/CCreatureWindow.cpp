#include "StdInc.h"
#include "CCreatureWindow.h"

#include "CGameInfo.h"
#include "CPlayerInterface.h"
#include "GUIClasses.h"

#include "../CCallback.h"
#include "../lib/BattleState.h"
#include "../lib/CBonusTypeHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CModHandler.h"
#include "../lib/CSpellHandler.h"

#include "gui/CGuiHandler.h"
#include "gui/CIntObjectClasses.h"

using namespace CSDL_Ext;

class CCreatureArtifactInstance;
class CSelectableSkill;

/*
 * CCreatureWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct StackWindowInfo
{
	// helper structs
	struct CommanderLevelInfo
	{
		std::vector<ui32> skills;
		std::function<void(ui32)> callback;
	};
	struct StackDismissInfo
	{
		std::function<void()> callback;
	};
	struct StackUpgradeInfo
	{
		UpgradeInfo info;
		std::function<void(CreatureID)> callback;
	};

	// pointers to permament objects in game state
	const CCreature * creature;
	const CCommanderInstance * commander;
	const CStackInstance * stackNode;
	const CGHeroInstance * owner;

	// temporary objects which should be kept as copy if needed
	boost::optional<CommanderLevelInfo> levelupInfo;
	boost::optional<StackDismissInfo> dismissInfo;
	boost::optional<StackUpgradeInfo> upgradeInfo;

	// misc fields
	unsigned int creatureCount;
	bool popupWindow;

	StackWindowInfo();
};

namespace
{
	namespace EStat
	{
		enum EStat
		{
			ATTACK,
			DEFENCE,
			SHOTS,
			DAMAGE,
			HEALTH,
			HEALTH_LEFT,
			SPEED,
			MANA
		};
	}
}

StackWindowInfo::StackWindowInfo():
	creature(nullptr),
	commander(nullptr),
	stackNode(nullptr),
	owner(nullptr),
	creatureCount(0),
	popupWindow(false)
{
}

void CStackWindow::CWindowSection::createBackground(std::string path)
{
	background = new CPicture("stackWindow/" + path);
	pos = background->pos;
}

void CStackWindow::CWindowSection::printStatString(int index, std::string name, std::string value)
{
	new CLabel(145, 32 + index*19, FONT_SMALL, TOPLEFT, Colors::WHITE, name);
	new CLabel(307, 48 + index*19, FONT_SMALL, BOTTOMRIGHT, Colors::WHITE, value);
}

void CStackWindow::CWindowSection::printStatRange(int index, std::string name, int min, int max)
{
	if(min != max)
		printStatString(index, name, boost::str(boost::format("%d - %d") % min % max));
	else
		printStatString(index, name, boost::str(boost::format("%d") % min));
}

void CStackWindow::CWindowSection::printStatBase(int index, std::string name, int base, int current)
{
	if(base != current)
		printStatString(index, name, boost::str(boost::format("%d (%d)") % base % current));
	else
		printStatString(index, name, boost::str(boost::format("%d") % base));
}

void CStackWindow::CWindowSection::printStat(int index, std::string name, int value)
{
	printStatBase(index, name, value, value);
}

void CStackWindow::CWindowSection::createStackInfo(bool showExp, bool showArt)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if (showExp && showArt)
		createBackground("info-panel-2");
	else if (showExp || showArt)
		createBackground("info-panel-1");
	else
		createBackground("info-panel-0");

	new CCreaturePic(5, 41, parent->info->creature);

	std::string visibleName;
	if (parent->info->commander != nullptr)
		visibleName = parent->info->commander->type->nameSing;
	else
		visibleName = parent->info->creature->namePl;
	new CLabel(215, 12, FONT_SMALL, CENTER, Colors::YELLOW, visibleName);

	int dmgMultiply = 1;
	if(parent->info->owner && parent->info->stackNode->hasBonusOfType(Bonus::SIEGE_WEAPON))
		dmgMultiply += parent->info->owner->Attack();

	new CPicture("stackWindow/icons", 117, 32);
	printStatBase(EStat::ATTACK, CGI->generaltexth->primarySkillNames[0], parent->info->creature->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK), parent->info->stackNode->Attack());
	printStatBase(EStat::DEFENCE, CGI->generaltexth->primarySkillNames[1], parent->info->creature->valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE), parent->info->stackNode->Defense());
	printStatRange(EStat::DAMAGE, CGI->generaltexth->allTexts[199], parent->info->stackNode->getMinDamage() * dmgMultiply, parent->info->stackNode->getMaxDamage() * dmgMultiply);
	printStatBase(EStat::HEALTH, CGI->generaltexth->allTexts[388], parent->info->creature->valOfBonuses(Bonus::STACK_HEALTH), parent->info->stackNode->valOfBonuses(Bonus::STACK_HEALTH));
	printStatBase(EStat::SPEED, CGI->generaltexth->zelp[441].first, parent->info->creature->Speed(), parent->info->stackNode->Speed());

	const CStack * battleStack = dynamic_cast<const CStack*>(parent->info->stackNode);
	bool shooter = parent->info->stackNode->hasBonusOfType(Bonus::SHOOTER) && parent->info->stackNode->valOfBonuses(Bonus::SHOTS);
	bool caster  = parent->info->stackNode->valOfBonuses(Bonus::CASTS);

	if (battleStack != nullptr) // in battle
	{
		if (shooter)
			printStatBase(EStat::SHOTS, CGI->generaltexth->allTexts[198], battleStack->valOfBonuses(Bonus::SHOTS), battleStack->shots);
		if (caster)
			printStatBase(EStat::MANA, CGI->generaltexth->allTexts[399], battleStack->valOfBonuses(Bonus::CASTS), battleStack->casts);
		printStat(EStat::HEALTH_LEFT, CGI->generaltexth->allTexts[200], battleStack->firstHPleft);
	}
	else
	{
		if (shooter)
			printStat(EStat::SHOTS, CGI->generaltexth->allTexts[198], parent->info->stackNode->valOfBonuses(Bonus::SHOTS));
		if (caster)
			printStat(EStat::MANA, CGI->generaltexth->allTexts[399], parent->info->stackNode->valOfBonuses(Bonus::CASTS));
	}

	auto morale = new MoraleLuckBox(true, genRect(42, 42, 321, 110));
	morale->set(parent->info->stackNode);
	auto luck = new MoraleLuckBox(false, genRect(42, 42, 375, 110));
	luck->set(parent->info->stackNode);
}

void CStackWindow::CWindowSection::createActiveSpells()
{
	static const Point firstPos(7 ,4); // position of 1st spell box
	static const Point offset(54, 0);  // offset of each spell box from previous

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	createBackground("spell-effects");

	const CStack * battleStack = dynamic_cast<const CStack*>(parent->info->stackNode);

	assert(battleStack); // Section should be created only for battles

	//spell effects
	int printed=0; //how many effect pics have been printed
	std::vector<si32> spells = battleStack->activeSpells();
	for(si32 effect : spells)
	{
		std::string spellText;
		if (effect < 77) //not all effects have graphics (for eg. Acid Breath)
		{
			spellText = CGI->generaltexth->allTexts[610]; //"%s, duration: %d rounds."
			boost::replace_first (spellText, "%s", CGI->spellh->objects[effect]->name);
			int duration = battleStack->getBonusLocalFirst(Selector::source(Bonus::SPELL_EFFECT,effect))->turnsRemain;
			boost::replace_first (spellText, "%d", boost::lexical_cast<std::string>(duration));

			new CAnimImage("SpellInt", effect + 1, 0, firstPos.x + offset.x * printed, firstPos.x + offset.y * printed);
			new LRClickableAreaWText(Rect(firstPos + offset * printed, Point(50, 38)), spellText, spellText);
			if (++printed >= 8) // interface limit reached
				break;
		}
	}
}

void CStackWindow::CWindowSection::createCommanderSection()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	auto onCreate = [=](size_t index) -> CIntObject *
	{
		return parent->switchTab(index);
	};
	auto onDestroy = [=](CIntObject * obj)
	{
		delete obj;
	};
	parent->commanderTab = new CTabbedInt(onCreate, onDestroy, Point(0,0), 0);
	pos.w = parent->pos.w;
	pos.h = 177; //fixed height
}

static std::string skillToFile (int skill, int level, bool selected)
{
	// FIXME: is this a correct hadling?
	// level 0 = skill not present, use image with "no" suffix
	// level 1-5 = skill available, mapped to images indexed as 0-4
	// selecting skill means that it will appear one level higher (as if alredy upgraded)
	std::string file = "zvs/Lib1.res/_";
	switch (skill)
	{
		case ECommander::ATTACK:
			file += "AT";
			break;
		case ECommander::DEFENSE:
			file += "DF";
			break;
		case ECommander::HEALTH:
			file += "HP";
			break;
		case ECommander::DAMAGE:
			file += "DM";
			break;
		case ECommander::SPEED:
			file += "SP";
			break;
		case ECommander::SPELL_POWER:
			file += "MP";
			break;
	}
	std::string sufix;
	if (selected)
		level++; // UI will display resulting level
	if (level == 0)
		sufix = "no"; //not avaliable - no number
	else
		sufix = boost::lexical_cast<std::string>(level-1);
	if (selected)
		sufix += "="; //level-up highlight

	return file + sufix + ".bmp";
}

void CStackWindow::CWindowSection::createCommander()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	createBackground("commander-bg");

	auto getSkillPos = [&](int index)
	{
		return Point(10 + 80 * (index%3), 20 + 80 * (index/3));
	};

	auto getSkillImage = [this](int skillIndex) -> std::string
	{
		bool selected = ((parent->selectedSkill == skillIndex) && parent->info->levelupInfo );
		return skillToFile(skillIndex, parent->info->commander->secondarySkills[skillIndex], selected);
	};

	for (int index = ECommander::ATTACK; index <= ECommander::SPELL_POWER; ++index)
	{
		Point skillPos = getSkillPos(index);

		auto icon = new CClickableObject(new CPicture(getSkillImage(index), skillPos.x, skillPos.y), [=]{});

		if (parent->selectedSkill == index)
			parent->selectedIcon = icon;

		if (parent->info->levelupInfo && vstd::contains(parent->info->levelupInfo->skills, index)) // can be upgraded - enable selection switch
		{
			icon->callback = [=]
			{
				OBJ_CONSTRUCTION_CAPTURING_ALL;
				int oldSelection = parent->selectedSkill; // update selection
				parent->selectedSkill = index;

				if (parent->selectedIcon) // recreate image on old selection
					parent->selectedIcon->setObject(new CPicture(getSkillImage(oldSelection)));

				parent->selectedIcon = icon; // update new selection
				icon->setObject(new CPicture(getSkillImage(index)));
			};
		}
	}
}

void CStackWindow::CWindowSection::createCommanderAbilities()
{/*
	for (auto option : parent->info->levelupInfo->skills)
	{
		if (option >= 100) // this is an ability
		{

		}
	}
	selectableSkill->pos = Rect (95, 256, 55, 55); //TODO: scroll
	const Bonus *b = CGI->creh->skillRequirements[option-100].first;
	bonusItems.push_back (new CBonusItem (genRect(0, 0, 251, 57), stack->bonusToString(b, false), stack->bonusToString(b, true), stack->bonusToGraphics(b)));
	selectableBonuses.push_back (selectableSkill); //insert these before other bonuses
*/
}

void CStackWindow::CWindowSection::createBonuses(boost::optional<size_t> preferredSize)
{
	// size of single image for an item
	static const int itemHeight = 59;
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	size_t totalSize = (parent->activeBonuses.size() + 1) / 2;
	size_t visibleSize = preferredSize ? preferredSize.get() : std::min<size_t>(3, totalSize);

	pos.w = parent->pos.w;
	pos.h = itemHeight * visibleSize;

	auto onCreate = [=](size_t index) -> CIntObject *
	{
		return parent->createBonusEntry(index);
	};
	auto onDestroy = [=](CIntObject * obj)
	{
		delete obj;
	};
	new CListBox(onCreate, onDestroy, Point(0, 0), Point(0, itemHeight), visibleSize, totalSize, 0, 1, Rect(pos.w - 15, 0, pos.h, pos.h));
}

void CStackWindow::CWindowSection::createButtonPanel()
{
	//TODO: localization, place creature icon on button, proper path to animation-button
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	createBackground("button-panel");

	if (parent->info->dismissInfo)
	{
		auto onDismiss = [=]()
		{
			parent->info->dismissInfo->callback();
			parent->close();
		};
		auto onClick = [=] ()
		{
			LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[12], onDismiss, 0, false, std::vector<CComponent*>());
		};
		new CAdventureMapButton(CGI->generaltexth->zelp[445], onClick, 5, 5,"IVIEWCR2.DEF",SDLK_d);
	}
	if (parent->info->upgradeInfo)
	{
		// used space overlaps with commander switch button
		// besides - should commander really be upgradeable?
		assert(!parent->info->commander);

		StackWindowInfo::StackUpgradeInfo & upgradeInfo = parent->info->upgradeInfo.get();
		size_t buttonsToCreate = std::min<size_t>(upgradeInfo.info.newID.size(), 3); // no more than 3 windows on UI - space limit

		for (size_t i=0; i<buttonsToCreate; i++)
		{
			TResources totalCost = upgradeInfo.info.cost[i] * parent->info->creatureCount;

			auto onUpgrade = [=]()
			{
				upgradeInfo.callback(upgradeInfo.info.newID[i]);
				parent->close();
			};
			auto onClick = [=]()
			{
				std::vector<CComponent*> resComps;
				for(TResources::nziterator i(totalCost); i.valid(); i++)
				{
					resComps.push_back(new CComponent(CComponent::resource, i->resType, i->resVal));
				}
				LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[207], onUpgrade, nullptr, true, resComps);
			};
			auto upgradeBtn = new CAdventureMapButton(CGI->generaltexth->zelp[446], onClick, 221 + i * 40, 5, "stackWindow/upgradeButton", SDLK_1);

			upgradeBtn->addOverlay(new CAnimImage("CPRSMALL", VLC->creh->creatures[upgradeInfo.info.newID[i]]->iconIndex));

			if (!LOCPLINT->cb->getResourceAmount().canAfford(totalCost))
				upgradeBtn->block(true);
		}
	}

	if (parent->info->commander)
	{
		bool createAbilitiesTab = false;
		if (parent->info->levelupInfo)
		{
			for (auto option : parent->info->levelupInfo->skills)
			{
				if (option >= 100)
					createAbilitiesTab = true;
			}
		}

		for (size_t i=0; i<3; i++)
		{
			auto onSwitch = [&, i]()
			{
				parent->switchButtons[parent->activeTab]->enable();
				parent->commanderTab->setActive(i);
				parent->switchButtons[i]->disable();
				parent->redraw(); // FIXME: enable/disable don't redraw screen themselves
			};

			parent->switchButtons[i] = new CAdventureMapButton(std::make_pair("",""), onSwitch, 262 + i*40, 5, "stackWindow/upgradeButton", SDLK_1 + i);
			parent->switchButtons[i]->addOverlay(new CAnimImage("stackWindow/switchModeIcons", i));
		}

		parent->switchButtons[parent->activeTab]->disable();
		if (!createAbilitiesTab)
			parent->switchButtons[2]->disable();
	}

	auto exitBtn = new CAdventureMapButton(CGI->generaltexth->zelp[445], [=]{ parent->close(); }, 382, 5, "hsbtns.def", SDLK_RETURN);
	exitBtn->assignedKeys.insert(SDLK_ESCAPE);
}

CStackWindow::CWindowSection::CWindowSection(CStackWindow * parent):
	parent(parent)
{
}

CClickableObject::CClickableObject(CIntObject *object, std::function<void()> callback):
	object(nullptr),
	callback(callback)
{
	pos = object->pos;
	setObject(object);
}

void CClickableObject::setObject(CIntObject *newObject)
{
	delete object;
	object = newObject;
	addChild(object);
	object->moveTo(pos.topLeft());
	redraw();
}

void CClickableObject::clickLeft(tribool down, bool previousState)
{
	if (down)
		callback();
}

CIntObject * CStackWindow::createBonusEntry(size_t index)
{
	auto section = new CWindowSection(this);
	section->createBonusEntry(index);
	return section;
}

void CStackWindow::CWindowSection::createBonusEntry(size_t index)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	createBackground("bonus-effects");
	createBonusItem(index * 2, Point(6, 4));
	createBonusItem(index * 2 + 1, Point(214, 4));
}

void CStackWindow::CWindowSection::createBonusItem(size_t index, Point position)
{
	if (parent->activeBonuses.size() > index)
	{
		BonusInfo & bi = parent->activeBonuses[index];
		new CPicture(bi.imagePath, position.x, position.y);
		new CLabel(position.x + 60, position.y + 2,  FONT_SMALL, TOPLEFT, Colors::WHITE, bi.name);
		new CLabel(position.x + 60, position.y + 25, FONT_SMALL, TOPLEFT, Colors::WHITE, bi.description);
	}
}

CIntObject * CStackWindow::switchTab(size_t index)
{
	switch (index)
	{
		case 0:
		{
			activeTab = 0;
			auto ret = new CWindowSection(this);
			ret->createCommander();
			return ret;
		}
		case 1:
		{
			activeTab = 1;
			auto ret = new CWindowSection(this);
			ret->createBonuses(3);
			return ret;
		}
		case 2:
		{
			activeTab = 2;
			auto ret = new CWindowSection(this);
			ret->createCommanderAbilities();
			return ret;
		}
		default:
		{
			return nullptr;
		}
	}
}

void CStackWindow::initSections()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	CWindowSection * currentSection;

	bool showArt = CGI->modh->modules.STACK_ARTIFACT && info->commander == nullptr;
	bool showExp = CGI->modh->modules.STACK_EXP || info->commander != nullptr;
	currentSection = new CWindowSection(this);
	currentSection->createStackInfo(showExp, showArt);
	pos.w = currentSection->pos.w;
	pos.h += currentSection->pos.h;

	if (dynamic_cast<const CStack*>(info->stackNode)) // in battle
	{
		currentSection = new CWindowSection(this);
		currentSection->pos.y += pos.h;
		currentSection->createActiveSpells();
		pos.h += currentSection->pos.h;
	}
	if (info->commander)
	{
		currentSection = new CWindowSection(this);
		currentSection->pos.y += pos.h;
		currentSection->createCommanderSection();
		pos.h += currentSection->pos.h;
	}
	if (!info->commander && !activeBonuses.empty())
	{
		currentSection = new CWindowSection(this);
		currentSection->pos.y += pos.h;
		currentSection->createBonuses();
		pos.h += currentSection->pos.h;
	}

	if (!info->popupWindow)
	{
		currentSection = new CWindowSection(this);
		currentSection->pos.y += pos.h;
		currentSection->createButtonPanel();
		pos.h += currentSection->pos.h;
		//FIXME: add status bar to image?
	}
	updateShadow();
	pos = center(pos);
}

void CStackWindow::initBonusesList()
{
	BonusList output, input;
	input = *(info->stackNode->getBonuses(Selector::durationType(Bonus::PERMANENT).And(Selector::anyRange())));

	while (!input.empty())
	{
		Bonus * b = input.front();

		output.push_back(new Bonus(*b));
		output.back()->val = input.valOfBonuses(Selector::typeSubtype(b->type, b->subtype)); //merge multiple bonuses into one
		input.remove_if (Selector::typeSubtype(b->type, b->subtype)); //remove used bonuses
	}

	BonusInfo bonusInfo;
	for(Bonus* b : output)
	{
		bonusInfo.name = info->stackNode->bonusToString(b, false);
		bonusInfo.imagePath = info->stackNode->bonusToGraphics(b);

		//if it's possible to give any description or image for this kind of bonus
		//TODO: figure out why half of bonuses don't have proper description
		if (!bonusInfo.name.empty() || !bonusInfo.imagePath.empty())
			activeBonuses.push_back(bonusInfo);
	}

	//handle Magic resistance separately :/
	int magicResistance = info->stackNode->magicResistance();

	if (magicResistance)
	{
		BonusInfo bonusInfo;
		Bonus b;
		b.type = Bonus::MAGIC_RESISTANCE;

		bonusInfo.name = VLC->getBth()->bonusToString(&b, info->stackNode, false);
		bonusInfo.description = VLC->getBth()->bonusToString(&b, info->stackNode, true);
		bonusInfo.imagePath = info->stackNode->bonusToGraphics(&b);
		activeBonuses.push_back(bonusInfo);
	}
}

void CStackWindow::init()
{
	selectedIcon = nullptr;
	selectedSkill = 0;
	if (info->levelupInfo)
		selectedSkill = info->levelupInfo->skills.front();

	commanderTab = nullptr;
	activeTab = 0;

	initBonusesList();
	initSections();
}

CStackWindow::CStackWindow(const CStack * stack, bool popup):
	CWindowObject(BORDERED | (popup ? RCLICK_POPUP : 0))
{
	info->stackNode = stack->base;
	info->creature = stack->type;
	info->creatureCount = stack->count;
	info->popupWindow = popup;
	init();
}

CStackWindow::CStackWindow(const CCreature * creature, bool popup):
	CWindowObject(BORDERED | (popup ? RCLICK_POPUP : 0)),
	info(new StackWindowInfo())
{
	info->stackNode = new CStackInstance(creature, 1); // FIXME: free data
	info->creature = creature;
	info->popupWindow = popup;
	init();
}

CStackWindow::CStackWindow(const CStackInstance * stack, bool popup):
	CWindowObject(BORDERED | (popup ? RCLICK_POPUP : 0)),
	info(new StackWindowInfo())
{
	info->stackNode = stack;
	info->creature = stack->type;
	info->creatureCount = stack->count;
	info->popupWindow = popup;
	init();
}

CStackWindow::CStackWindow(const CStackInstance * stack, std::function<void()> dismiss, const UpgradeInfo & upgradeInfo, std::function<void(CreatureID)> callback):
	CWindowObject(BORDERED),
	info(new StackWindowInfo())
{
	info->stackNode = stack;
	info->creature = stack->type;
	info->creatureCount = stack->count;

	info->upgradeInfo = StackWindowInfo::StackUpgradeInfo();
	info->dismissInfo = StackWindowInfo::StackDismissInfo();
	info->upgradeInfo->info = upgradeInfo;
	info->upgradeInfo->callback = callback;
	info->dismissInfo->callback = dismiss;
	init();
}

CStackWindow::CStackWindow(const CCommanderInstance * commander, bool popup):
	CWindowObject(BORDERED | (popup ? RCLICK_POPUP : 0)),
	info(new StackWindowInfo())
{
	info->stackNode = commander;
	info->creature = commander->type;
	info->commander = commander;
	info->creatureCount = 1;
	info->popupWindow = popup;
	init();
}

CStackWindow::CStackWindow(const CCommanderInstance * commander, std::vector<ui32> &skills, std::function<void(ui32)> callback):
	CWindowObject(BORDERED),
	info(new StackWindowInfo())
{
	info->stackNode = commander;
	info->creature = commander->type;
	info->commander = commander;
	info->creatureCount = 1;
	info->levelupInfo = StackWindowInfo::CommanderLevelInfo();
	info->levelupInfo->skills = skills;
	info->levelupInfo->callback = callback;
	init();
}

CStackWindow::~CStackWindow()
{
	if (info->levelupInfo)
	{
		info->levelupInfo->callback(selectedSkill);
	}
}
