#include "StdInc.h"
#include "CCreatureWindow.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../widgets/ObjectLists.h"
#include "../gui/CGuiHandler.h"

#include "../../CCallback.h"
#include "../../lib/BattleState.h"
#include "../../lib/CBonusTypeHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CModHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CSpellHandler.h"
#include "../../lib/CGameState.h"

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
	const CStack * stack;
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
	stack(nullptr),
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

std::string CStackWindow::generateStackExpDescription()
{
	const CStackInstance * stack = info->stackNode;
	const CCreature * creature = info->creature;

	int tier = stack->type->level;
	int rank = stack->getExpRank();
	if (!vstd::iswithin(tier, 1, 7))
		tier = 0;
	int number;
	std::string expText = CGI->generaltexth->zcrexp[325];
	boost::replace_first (expText, "%s", creature->namePl);
	boost::replace_first (expText, "%s", CGI->generaltexth->zcrexp[rank]);
	boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(rank));
	boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(stack->experience));
	number = CGI->creh->expRanks[tier][rank] - stack->experience;
	boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(number));

	number = CGI->creh->maxExpPerBattle[tier]; //percent
	boost::replace_first (expText, "%i%", boost::lexical_cast<std::string>(number));
	number *= CGI->creh->expRanks[tier].back() / 100; //actual amount
	boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(number));

	boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(stack->count)); //Number of Creatures in stack

	int expmin = std::max(CGI->creh->expRanks[tier][std::max(rank-1, 0)], (ui32)1);
	number = (stack->count * (stack->experience - expmin)) / expmin; //Maximum New Recruits without losing current Rank
	boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(number)); //TODO

	boost::replace_first (expText, "%.2f", boost::lexical_cast<std::string>(1)); //TODO Experience Multiplier
	number = CGI->creh->expAfterUpgrade;
	boost::replace_first (expText, "%.2f", boost::lexical_cast<std::string>(number) + "%"); //Upgrade Multiplier

	expmin = CGI->creh->expRanks[tier][9];
	int expmax = CGI->creh->expRanks[tier][10];
	number = expmax - expmin;
	boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(number)); //Experience after Rank 10
	number = (stack->count * (expmax - expmin)) / expmin;
	boost::replace_first (expText, "%i", boost::lexical_cast<std::string>(number)); //Maximum New Recruits to remain at Rank 10 if at Maximum Experience

	return expText;
}

void CStackWindow::removeStackArtifact(ArtifactPosition pos)
{
	auto art = info->stackNode->getArt(ArtifactPosition::CREATURE_SLOT);
	LOCPLINT->cb->swapArtifacts(ArtifactLocation(info->stackNode, pos),
								ArtifactLocation(info->owner, art->firstBackpackSlot(info->owner)));
	delete stackArtifactButton;
	delete stackArtifactHelp;
	delete stackArtifactIcon;
}

void CStackWindow::setStackArtifact(const CArtifactInstance * art, Point artPos)
{
	if (art)
	{
		stackArtifactIcon = new CAnimImage("ARTIFACT", art->artType->iconIndex, 0, pos.x, pos.y);
		stackArtifactHelp = new LRClickableAreaWTextComp(Rect(artPos, Point(44, 44)), CComponent::artifact);
		stackArtifactHelp->type = art->artType->id;

		const JsonNode & text = VLC->generaltexth->localizedTexts["creatureWindow"]["returnArtifact"];

		if (info->owner)
		{
			stackArtifactButton = new CButton(Point(artPos.x - 2 , artPos.y + 46), "stackWindow/cancelButton",
			                                  CButton::tooltip(text),
			                                  [=]{ removeStackArtifact(ArtifactPosition::CREATURE_SLOT); });
		}
	}

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

	auto pic = new CCreaturePic(5, 41, parent->info->creature);

	if (parent->info->stackNode != nullptr && parent->info->commander == nullptr)
	{
		//normal stack, not a commander and not non-existing stack (e.g. recruitment dialog)
		pic->setAmount(parent->info->stackNode->count);
	}

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

	const CStack * battleStack = parent->info->stack;
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

	if (showArt)
	{
		Point pos = showExp ? Point(375, 32) : Point(347, 32);
		parent->setStackArtifact(parent->info->stackNode->getArt(ArtifactPosition::CREATURE_SLOT), pos);
	}

	if (showExp)
	{
		const CStackInstance * stack = parent->info->stackNode;
		Point pos = showArt ? Point(321, 32) : Point(347, 32);
		if (parent->info->commander)
		{
			const CCommanderInstance * commander = parent->info->commander;
			new CAnimImage("PSKIL42", 4, 0, pos.x, pos.y); // experience icon

			auto expArea = new LRClickableAreaWTextComp(Rect(pos.x, pos.y, 44, 44), CComponent::experience);
			expArea->text = CGI->generaltexth->allTexts[2];
			expArea->bonusValue = commander->getExpRank();
			boost::replace_first(expArea->text, "%d", boost::lexical_cast<std::string>(commander->getExpRank()));
			boost::replace_first(expArea->text, "%d", boost::lexical_cast<std::string>(CGI->heroh->reqExp(commander->getExpRank()+1)));
			boost::replace_first(expArea->text, "%d", boost::lexical_cast<std::string>(commander->experience));
		}
		else
		{
			new CAnimImage("stackWindow/levels", stack->getExpRank(), 0, pos.x, pos.y);
			auto expArea = new LRClickableAreaWText(Rect(pos.x, pos.y, 44, 44));
			expArea->text = parent->generateStackExpDescription();
		}
		new CLabel(pos.x + 21, pos.y + 52, FONT_SMALL, CENTER, Colors::WHITE, makeNumberShort<TExpType>(stack->experience, 6));
	}
}

void CStackWindow::CWindowSection::createActiveSpells()
{
	static const Point firstPos(6, 2); // position of 1st spell box
	static const Point offset(54, 0);  // offset of each spell box from previous

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	createBackground("spell-effects");

	const CStack * battleStack = parent->info->stack;

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

			new CAnimImage("SpellInt", effect + 1, 0, firstPos.x + offset.x * printed, firstPos.y + offset.y * printed);
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
	pos.h = 177; // height of commander info
	if (parent->info->levelupInfo)
		pos.h += 59; // height of abilities selection
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
				parent->setSelection(index, icon);
			};
		}
	}
	//TODO: commander artifacts
}

CIntObject * CStackWindow::createSkillEntry(int index)
{
	for (auto skillID : info->levelupInfo->skills)
	{
		if (index == 0 && skillID >= 100)
		{
			const Bonus *bonus = CGI->creh->skillRequirements[skillID-100].first;
			const CStackInstance *stack = info->commander;
			CClickableObject * icon = new CClickableObject(new CPicture(stack->bonusToGraphics(bonus)), []{});
			icon->callback = [=]
			{
				setSelection(skillID, icon);
			};
			icon->text = stack->bonusToString(bonus, true);
			icon->hoverText = stack->bonusToString(bonus, false);
			return icon;
		}
		if (skillID >= 100)
			index--;
	}
	return nullptr;
}

void CStackWindow::CWindowSection::createCommanderAbilities()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	auto bg2 = new CPicture("stackWindow/commander-abilities.png");
	bg2->moveBy(Point(0, pos.h));
	size_t abilitiesCount = boost::range::count_if(parent->info->levelupInfo->skills, [](ui32 skillID)
	{
		return skillID >= 100;
	});

	auto list = new CListBox([=] (int index)
	{
		return parent->createSkillEntry(index);
	},
	[=] (CIntObject * elem)
	{
		delete elem;
	},
	Point(38, 3+pos.h), Point(63, 0), 6, abilitiesCount);

	auto leftBtn   = new CButton(Point(10,  pos.h + 6), "hsbtns3.def", CButton::tooltip(), [=]{ list->moveToPrev(); }, SDLK_LEFT);
	auto rightBtn =  new CButton(Point(411, pos.h + 6), "hsbtns5.def", CButton::tooltip(), [=]{ list->moveToNext(); }, SDLK_RIGHT);

	if (abilitiesCount <= 6)
	{
		leftBtn->block(true);
		rightBtn->block(true);
	}
}

void CStackWindow::setSelection(si32 newSkill, CClickableObject * newIcon)
{
	auto getSkillImage = [this](int skillIndex) -> std::string
	{
		bool selected = ((selectedSkill == skillIndex) && info->levelupInfo );
		return skillToFile(skillIndex, info->commander->secondarySkills[skillIndex], selected);
	};

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	int oldSelection = selectedSkill; // update selection
	selectedSkill = newSkill;

	if (selectedIcon && oldSelection < 100) // recreate image on old selection, only for skills
		selectedIcon->setObject(new CPicture(getSkillImage(oldSelection)));

	selectedIcon = newIcon; // update new selection
	if (newSkill < 100)
		newIcon->setObject(new CPicture(getSkillImage(newSkill)));
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
		new CButton(Point(5, 5),"IVIEWCR2.DEF", CGI->generaltexth->zelp[445], onClick, SDLK_d);
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
			auto upgradeBtn = new CButton(Point(221 + i * 40, 5), "stackWindow/upgradeButton", CGI->generaltexth->zelp[446], onClick, SDLK_1);

			upgradeBtn->addOverlay(new CAnimImage("CPRSMALL", VLC->creh->creatures[upgradeInfo.info.newID[i]]->iconIndex));

			if (!LOCPLINT->cb->getResourceAmount().canAfford(totalCost))
				upgradeBtn->block(true);
		}
	}

	if (parent->info->commander)
	{
		for (size_t i=0; i<2; i++)
		{
			std::string btnIDs[2] = { "showSkills", "showBonuses" };
			auto onSwitch = [&, i]()
			{
				parent->switchButtons[parent->activeTab]->enable();
				parent->commanderTab->setActive(i);
				parent->switchButtons[i]->disable();
				parent->redraw(); // FIXME: enable/disable don't redraw screen themselves
			};

			const JsonNode & text = VLC->generaltexth->localizedTexts["creatureWindow"][btnIDs[i]];
			parent->switchButtons[i] = new CButton(Point(302 + i*40, 5), "stackWindow/upgradeButton", CButton::tooltip(text), onSwitch);
			parent->switchButtons[i]->addOverlay(new CAnimImage("stackWindow/switchModeIcons", i));
		}
		parent->switchButtons[parent->activeTab]->disable();
	}

	auto exitBtn = new CButton(Point(382, 5), "hsbtns.def", CGI->generaltexth->zelp[445], [=]{ parent->close(); }, SDLK_RETURN);
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
		new CMultiLineLabel(Rect(position.x + 60, position.y + 17, 137,30), FONT_SMALL, TOPLEFT, Colors::WHITE, bi.description);
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
			if (info->levelupInfo)
				ret->createCommanderAbilities();
			return ret;
		}
		case 1:
		{
			activeTab = 1;
			auto ret = new CWindowSection(this);
			if (info->levelupInfo)
				ret->createBonuses(4);
			else
				ret->createBonuses(3);
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

	bool showArt = CGI->modh->modules.STACK_ARTIFACT && info->commander == nullptr && info->stackNode;
	bool showExp = (CGI->modh->modules.STACK_EXP || info->commander != nullptr) && info->stackNode;
	currentSection = new CWindowSection(this);
	currentSection->createStackInfo(showExp, showArt);
	pos.w = currentSection->pos.w;
	pos.h += currentSection->pos.h;

	if (info->stack) // in battle
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
		bonusInfo.description = info->stackNode->bonusToString(b, true);
		bonusInfo.imagePath = info->stackNode->bonusToGraphics(b);

		//if it's possible to give any description or image for this kind of bonus
		//TODO: figure out why half of bonuses don't have proper description
		if ((!bonusInfo.name.empty() || !bonusInfo.imagePath.empty())&& b->type != Bonus::MAGIC_RESISTANCE)
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
	if (!info->stackNode)
		info->stackNode = new CStackInstance(info->creature, 1);// FIXME: free data

	stackArtifactHelp = nullptr;
	stackArtifactIcon = nullptr;
	stackArtifactButton = nullptr;

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
	CWindowObject(BORDERED | (popup ? RCLICK_POPUP : 0)),
    info(new StackWindowInfo())
{
	info->stack = stack;
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
	info->owner = dynamic_cast<const CGHeroInstance *> (stack->armyObj);	
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
	info->owner = dynamic_cast<const CGHeroInstance *> (stack->armyObj);
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
	info->owner = dynamic_cast<const CGHeroInstance *> (commander->armyObj);	
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
	info->owner = dynamic_cast<const CGHeroInstance *> (commander->armyObj);		
	init();
}

CStackWindow::~CStackWindow()
{
	if (info->levelupInfo)
		info->levelupInfo->callback(vstd::find_pos(info->levelupInfo->skills, selectedSkill));
}
