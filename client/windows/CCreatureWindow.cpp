/*
 * CCreatureWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CCreatureWindow.h"

#include <vcmi/spells/Spell.h>
#include <vcmi/spells/Service.h>

#include "../CPlayerInterface.h"
#include "../render/Canvas.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/CComponentHolder.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../windows/InfoWindows.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../battle/BattleInterface.h"

#include "../../lib/CBonusTypeHandler.h"
#include "../../lib/CStack.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/bonuses/Propagators.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/entities/artifact/ArtifactUtils.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/gameState/UpgradeInfo.h"
#include "../../lib/networkPacks/ArtifactLocation.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/TextOperations.h"
#include "../../lib/texts/Languages.h"

class CCreatureArtifactInstance;
class CSelectableSkill;

class UnitView
{
public:
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
		StackUpgradeInfo() = delete;
		StackUpgradeInfo(const UpgradeInfo & upgradeInfo)
			: info(upgradeInfo)
		{ }
		UpgradeInfo info;
		std::function<void(CreatureID)> callback;
	};

	// pointers to permanent objects in game state
	const CCreature * creature;
	const CCommanderInstance * commander;
	const CStackInstance * stackNode;
	const CStack * stack;
	const CGHeroInstance * owner;

	// temporary objects which should be kept as copy if needed
	std::optional<CommanderLevelInfo> levelupInfo;
	std::optional<StackDismissInfo> dismissInfo;
	std::optional<StackUpgradeInfo> upgradeInfo;

	// misc fields
	unsigned int creatureCount;
	bool popupWindow;

	UnitView()
		: creature(nullptr),
		commander(nullptr),
		stackNode(nullptr),
		stack(nullptr),
		owner(nullptr),
		creatureCount(0),
		popupWindow(false)
	{
	}

	std::string getName() const
	{
		if(commander)
			return commander->getType()->getNameSingularTranslated();
		else
			return creature->getNamePluralTranslated();
	}
private:

};

CCommanderSkillIcon::CCommanderSkillIcon(std::shared_ptr<CIntObject> object_, bool isMasterAbility_, std::function<void()> callback)
	: object(),
	  isMasterAbility(isMasterAbility_),
	  isSelected(false),
	  callback(callback)
{
	pos = object_->pos;
	this->isMasterAbility = isMasterAbility_;
	setObject(object_);
}

void CCommanderSkillIcon::setObject(std::shared_ptr<CIntObject> newObject)
{
	if(object)
		removeChild(object.get());
	object = newObject;
	addChild(object.get());
	object->moveTo(pos.topLeft());
	redraw();
}

void CCommanderSkillIcon::clickPressed(const Point & cursorPosition)
{
	callback();
	isSelected = true;
	redraw();
}

void CCommanderSkillIcon::deselect()
{
	isSelected = false;
	redraw();
}

bool CCommanderSkillIcon::getIsMasterAbility()
{
	return isMasterAbility;
}

void CCommanderSkillIcon::show(Canvas &to)
{
	CIntObject::show(to);

	if(isMasterAbility && isSelected)
		to.drawBorder(pos, Colors::YELLOW, 2);
}

static ImagePath skillToFile(int skill, int level, bool selected)
{
	// FIXME: is this a correct handling?
	// level 0 = skill not present, use image with "no" suffix
	// level 1-5 = skill available, mapped to images indexed as 0-4
	// selecting skill means that it will appear one level higher (as if already upgraded)
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
	std::string suffix;
	if (selected)
		level++; // UI will display resulting level
	if (level == 0)
		suffix = "no"; //not available - no number
	else
		suffix = std::to_string(level-1);
	if (selected)
		suffix += "="; //level-up highlight

	return ImagePath::builtin(file + suffix + ".bmp");
}

CStackWindow::CWindowSection::CWindowSection(CStackWindow * parent, const ImagePath & backgroundPath, int yOffset)
	: parent(parent)
{
	pos.y += yOffset;
	OBJECT_CONSTRUCTION;
	if(!backgroundPath.empty())
	{
		background = std::make_shared<CPicture>(backgroundPath);
		pos.w = background->pos.w;
		pos.h = background->pos.h;
	}
}

CStackWindow::ActiveSpellsSection::ActiveSpellsSection(CStackWindow * owner, int yOffset)
	: CWindowSection(owner, ImagePath::builtin("stackWindow/spell-effects"), yOffset)
{
	static const Point firstPos(6, 2); // position of 1st spell box
	static const Point offset(54, 0);  // offset of each spell box from previous

	OBJECT_CONSTRUCTION;

	const CStack * battleStack = parent->info->stack;

	assert(battleStack); // Section should be created only for battles

	//spell effects
	int printed=0; //how many effect pics have been printed
	std::vector<SpellID> spells = battleStack->activeSpells();
	for(SpellID effect : spells)
	{
		const spells::Spell * spell = LIBRARY->spells()->getById(effect);

		//not all effects have graphics (for eg. Acid Breath)
		//for modded spells iconEffect is added to SpellInt.def
		const bool hasGraphics = (effect < SpellID::THUNDERBOLT) || (effect >= SpellID::AFTER_LAST);

		if (hasGraphics)
		{
			auto spellBonuses = battleStack->getBonuses(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(effect)));
			if (spellBonuses->empty())
				throw std::runtime_error("Failed to find effects for spell " + effect.toSpell()->getJsonKey());

			int duration = spellBonuses->front()->turnsRemain;
			std::string preferredLanguage = LIBRARY->generaltexth->getPreferredLanguage();

			MetaString spellText;
			spellText.appendTextID(spell->getDescriptionTextID(0)); // TODO: select correct mastery level?
			spellText.appendRawString("\n");
			spellText.appendTextID(Languages::getPluralFormTextID( preferredLanguage, duration, "vcmi.battleResultsWindow.spellDurationRemaining"));
			spellText.replaceNumber(duration);
			std::string spellDescription = spellText.toString();

			spellIcons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("SpellInt"), effect + 1, 0, firstPos.x + offset.x * printed, firstPos.y + offset.y * printed));
			labels.push_back(std::make_shared<CLabel>(firstPos.x + offset.x * printed + 46, firstPos.y + offset.y * printed + 36, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(duration)));
			clickableAreas.push_back(std::make_shared<LRClickableAreaWText>(Rect(firstPos + offset * printed, Point(50, 38)), spellDescription, spellDescription));
			if(++printed >= 8) // interface limit reached
				break;
		}
	}
}

CStackWindow::BonusLineSection::BonusLineSection(CStackWindow * owner, size_t lineIndex)
	: CWindowSection(owner, ImagePath::builtin("stackWindow/bonus-effects"), 0)
{
	OBJECT_CONSTRUCTION;

	static const std::array<Point, 2> offset =
	{
		Point(6, 2),
		Point(214, 2)
	};

	auto drawBonusSource = [this](int leftRight, Point p, BonusInfo & bi)
	{
		std::map<BonusSource, ColorRGBA> bonusColors = {
			{BonusSource::ARTIFACT,          Colors::GREEN},
			{BonusSource::ARTIFACT_INSTANCE, Colors::GREEN},
			{BonusSource::CREATURE_ABILITY,  Colors::YELLOW},
			{BonusSource::SPELL_EFFECT,      Colors::ORANGE},
			{BonusSource::SECONDARY_SKILL,   Colors::PURPLE},
			{BonusSource::HERO_SPECIAL,      Colors::PURPLE},
			{BonusSource::STACK_EXPERIENCE,  Colors::CYAN},
			{BonusSource::COMMANDER,         Colors::CYAN},
		};

		std::map<BonusSource, std::string> bonusNames = {
			{BonusSource::ARTIFACT,          LIBRARY->generaltexth->translate("vcmi.bonusSource.artifact")},
			{BonusSource::ARTIFACT_INSTANCE, LIBRARY->generaltexth->translate("vcmi.bonusSource.artifact")},
			{BonusSource::CREATURE_ABILITY,  LIBRARY->generaltexth->translate("vcmi.bonusSource.creature")},
			{BonusSource::SPELL_EFFECT,      LIBRARY->generaltexth->translate("vcmi.bonusSource.spell")},
			{BonusSource::SECONDARY_SKILL,   LIBRARY->generaltexth->translate("vcmi.bonusSource.hero")},
			{BonusSource::HERO_SPECIAL,      LIBRARY->generaltexth->translate("vcmi.bonusSource.hero")},
			{BonusSource::STACK_EXPERIENCE,  LIBRARY->generaltexth->translate("vcmi.bonusSource.commander")},
			{BonusSource::COMMANDER,         LIBRARY->generaltexth->translate("vcmi.bonusSource.commander")},
		};

		auto c = bonusColors.count(bi.bonusSource) ? bonusColors[bi.bonusSource] : ColorRGBA(192, 192, 192);
		std::string t = bonusNames.count(bi.bonusSource) ? bonusNames[bi.bonusSource] : LIBRARY->generaltexth->translate("vcmi.bonusSource.other");
		int maxLen = 50;
		EFonts f = FONT_TINY;
		Point pText = p + Point(4, 38);

		// 1px Black border
		bonusSource[leftRight].push_back(std::make_shared<CLabel>(pText.x - 1, pText.y, f, ETextAlignment::TOPLEFT, Colors::BLACK, t, maxLen));
		bonusSource[leftRight].push_back(std::make_shared<CLabel>(pText.x + 1, pText.y, f, ETextAlignment::TOPLEFT, Colors::BLACK, t, maxLen));
		bonusSource[leftRight].push_back(std::make_shared<CLabel>(pText.x, pText.y - 1, f, ETextAlignment::TOPLEFT, Colors::BLACK, t, maxLen));
		bonusSource[leftRight].push_back(std::make_shared<CLabel>(pText.x, pText.y + 1, f, ETextAlignment::TOPLEFT, Colors::BLACK, t, maxLen));
		bonusSource[leftRight].push_back(std::make_shared<CLabel>(pText.x, pText.y, f, ETextAlignment::TOPLEFT, c, t, maxLen));

		frame[leftRight] = std::make_shared<GraphicalPrimitiveCanvas>(Rect(p.x, p.y, 52, 52));
		frame[leftRight]->addRectangle(Point(0, 0), Point(52, 52), c);
	};

	for(size_t leftRight : {0, 1})
	{
		auto position = offset[leftRight];
		size_t bonusIndex = lineIndex * 2 + leftRight;

		if(parent->activeBonuses.size() > bonusIndex)
		{
			BonusInfo & bi = parent->activeBonuses[bonusIndex];
			if (!bi.imagePath.empty())
				icon[leftRight] = std::make_shared<CPicture>(bi.imagePath, position.x, position.y);

			description[leftRight] = std::make_shared<CMultiLineLabel>(Rect(position.x + 60, position.y, 137, 50), FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, bi.description);
			drawBonusSource(leftRight, Point(position.x - 1, position.y - 1), bi);
		}
	}
}

CStackWindow::BonusesSection::BonusesSection(CStackWindow * owner, int yOffset, std::optional<size_t> preferredSize):
	CWindowSection(owner, {}, yOffset)
{
	OBJECT_CONSTRUCTION;

	// size of single image for an item
	static const int itemHeight = 59;

	size_t totalSize = (owner->activeBonuses.size() + 1) / 2;
	size_t visibleSize = preferredSize.value_or(std::min<size_t>(3, totalSize));

	pos.w = owner->pos.w;
	pos.h = itemHeight * (int)visibleSize;

	auto onCreate = [=](size_t index) -> std::shared_ptr<CIntObject>
	{
		return std::make_shared<BonusLineSection>(owner, index);
	};

	lines = std::make_shared<CListBox>(onCreate, Point(0, 0), Point(0, itemHeight), visibleSize, totalSize, 0, totalSize > 3 ? 1 : 0, Rect(pos.w - 15, 0, pos.h, pos.h));
	lines->onScroll = [owner](){ owner->redraw(); };
}

CStackWindow::ButtonsSection::ButtonsSection(CStackWindow * owner, int yOffset)
	: CWindowSection(owner, ImagePath::builtin("stackWindow/button-panel"), yOffset)
{
	OBJECT_CONSTRUCTION;

	if(parent->info->dismissInfo && parent->info->dismissInfo->callback)
	{
		auto onDismiss = [this]()
		{
			parent->info->dismissInfo->callback();
			parent->close();
		};
		auto onClick = [=] ()
		{
			GAME->interface()->showYesNoDialog(LIBRARY->generaltexth->allTexts[12], onDismiss, nullptr);
		};
		dismiss = std::make_shared<CButton>(Point(5, 5),AnimationPath::builtin("IVIEWCR2.DEF"), LIBRARY->generaltexth->zelp[445], onClick, EShortcut::HERO_DISMISS);
	}

	if(parent->info->upgradeInfo && !parent->info->commander)
	{
		// used space overlaps with commander switch button
		// besides - should commander really be upgradeable?

		auto & upgradeInfo = parent->info->upgradeInfo.value();
		const size_t buttonsToCreate = std::min<size_t>(upgradeInfo.info.size(), upgrade.size());

		for(size_t buttonIndex = 0; buttonIndex < buttonsToCreate; buttonIndex++)
		{
			TResources totalCost = upgradeInfo.info.getAvailableUpgradeCosts().at(buttonIndex) * parent->info->creatureCount;

			auto onUpgrade = [this, upgradeInfo, buttonIndex]()
			{
				upgradeInfo.callback(upgradeInfo.info.getAvailableUpgrades().at(buttonIndex));
				parent->close();
			};
			auto onClick = [totalCost, onUpgrade]()
			{
				std::vector<std::shared_ptr<CComponent>> resComps;
				for(TResources::nziterator i(totalCost); i.valid(); i++)
				{
					resComps.push_back(std::make_shared<CComponent>(ComponentType::RESOURCE, i->resType, i->resVal));
				}

				if(GAME->interface()->cb->getResourceAmount().canAfford(totalCost))
				{
					GAME->interface()->showYesNoDialog(LIBRARY->generaltexth->allTexts[207], onUpgrade, nullptr, resComps);
				}
				else
				{
					GAME->interface()->showInfoDialog(LIBRARY->generaltexth->allTexts[314], resComps);
				}
			};
			auto upgradeBtn = std::make_shared<CButton>(Point(221 + (int)buttonIndex * 40, 5), AnimationPath::builtin("stackWindow/upgradeButton"), LIBRARY->generaltexth->zelp[446], onClick);

			upgradeBtn->setOverlay(std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), LIBRARY->creh->objects[upgradeInfo.info.getAvailableUpgrades()[buttonIndex]]->getIconIndex()));

			if(buttonsToCreate == 1) // single upgrade available
				upgradeBtn->assignedKey = EShortcut::RECRUITMENT_UPGRADE;

			upgrade[buttonIndex] = upgradeBtn;
		}
	}

	if(parent->info->commander)
	{
		for(size_t buttonIndex = 0; buttonIndex < 2; buttonIndex++)
		{
			std::string btnIDs[2] = { "showSkills", "showBonuses" };
			auto onSwitch = [buttonIndex, this]()
			{
				logAnim->debug("Switch %d->%d", parent->activeTab, buttonIndex);

				parent->switchButtons[parent->activeTab]->enable();
				parent->commanderTab->setActive(buttonIndex);
				parent->switchButtons[buttonIndex]->disable();
				parent->redraw(); // FIXME: enable/disable don't redraw screen themselves
			};

			std::string tooltipText = "vcmi.creatureWindow." + btnIDs[buttonIndex];
			parent->switchButtons[buttonIndex] = std::make_shared<CButton>(Point(342, 5), AnimationPath::builtin("stackWindow/upgradeButton"), CButton::tooltipLocalized(tooltipText), onSwitch);
			parent->switchButtons[buttonIndex]->setOverlay(std::make_shared<CAnimImage>(AnimationPath::builtin("stackWindow/switchModeIcons"), buttonIndex));
		}
		parent->switchButtons[parent->activeTab]->disable();
	}

	exit = std::make_shared<CButton>(Point(382, 5), AnimationPath::builtin("hsbtns.def"), LIBRARY->generaltexth->zelp[447], [this](){ parent->close(); }, EShortcut::GLOBAL_RETURN);
}

CStackWindow::CommanderMainSection::CommanderMainSection(CStackWindow * owner, int yOffset)
	: CWindowSection(owner, ImagePath::builtin("stackWindow/commander-bg"), yOffset)
{
	OBJECT_CONSTRUCTION;

	auto getSkillPos = [](int index)
	{
		return Point(10 + 80 * (index%3), 20 + 80 * (index/3));
	};

	auto getSkillImage = [this](int skillIndex)
	{
		bool selected = ((parent->selectedSkill == skillIndex) && parent->info->levelupInfo );
		return skillToFile(skillIndex, parent->info->commander->secondarySkills[skillIndex], selected);
	};

	auto getSkillDescription = [this](int skillIndex) -> std::string
	{
		return parent->getCommanderSkillDescription(skillIndex, parent->info->commander->secondarySkills[skillIndex]);
	};

	for(int index = ECommander::ATTACK; index <= ECommander::SPELL_POWER; ++index)
	{
		Point skillPos = getSkillPos(index);

		auto icon = std::make_shared<CCommanderSkillIcon>(std::make_shared<CPicture>(getSkillImage(index), skillPos.x, skillPos.y), false, [=]()
		{
			GAME->interface()->showInfoDialog(getSkillDescription(index));
		});

		icon->text = getSkillDescription(index); //used to handle right click description via LRClickableAreaWText::ClickRight()

		if(parent->selectedSkill == index)
			parent->selectedIcon = icon;

		if(parent->info->levelupInfo && vstd::contains(parent->info->levelupInfo->skills, index)) // can be upgraded - enable selection switch
		{
			if(parent->selectedSkill == index)
				parent->setSelection(index, icon);

			icon->callback = [this, index, icon]()
			{
				parent->setSelection(index, icon);
			};
		}

		skillIcons.push_back(icon);
	}

	auto getArtifactPos = [](int index)
	{
		return Point(269 + 52 * (index % 3), 22 + 52 * (index / 3));
	};

	for(auto equippedArtifact : parent->info->commander->artifactsWorn)
	{
		Point artPos = getArtifactPos(equippedArtifact.first);
		const auto commanderArt = equippedArtifact.second.getArt();
		assert(commanderArt);
		auto artPlace = std::make_shared<CCommanderArtPlace>(artPos, parent->info->owner, equippedArtifact.first, commanderArt->getTypeId());
		artifacts.push_back(artPlace);
	}

	if(parent->info->levelupInfo)
	{
		abilitiesBackground = std::make_shared<CPicture>(ImagePath::builtin("stackWindow/commander-abilities.png"));
		abilitiesBackground->moveBy(Point(0, pos.h));

		size_t abilitiesCount = boost::range::count_if(parent->info->levelupInfo->skills, [](ui32 skillID)
		{
			return skillID >= 100;
		});

		auto onCreate = [this](size_t index)->std::shared_ptr<CIntObject>
		{
			for(auto skillID : parent->info->levelupInfo->skills)
			{
				if(index == 0 && skillID >= 100)
				{
					const auto bonuses = LIBRARY->creh->skillRequirements[skillID-100].first;
					const CStackInstance * stack = parent->info->commander;
					auto icon = std::make_shared<CCommanderSkillIcon>(std::make_shared<CPicture>(stack->bonusToGraphics(bonuses[0])), true, [](){});
					icon->callback = [this, skillID, icon]()
					{
						parent->setSelection(skillID, icon);
					};
					for(int i = 0; i < bonuses.size(); i++) 
					{
						icon->hoverText += stack->bonusToString(bonuses[i]);
					}

					return icon;
				}
				if(skillID >= 100)
					index--;
			}
			return nullptr;
		};

		abilities = std::make_shared<CListBox>(onCreate, Point(38, 3+pos.h), Point(63, 0), 6, abilitiesCount);
		abilities->onScroll = [owner](){ owner->redraw(); };

		leftBtn = std::make_shared<CButton>(Point(10,  pos.h + 6), AnimationPath::builtin("hsbtns3.def"), CButton::tooltip(), [this](){ abilities->moveToPrev(); }, EShortcut::MOVE_LEFT);
		rightBtn = std::make_shared<CButton>(Point(411, pos.h + 6), AnimationPath::builtin("hsbtns5.def"), CButton::tooltip(), [this](){ abilities->moveToNext(); }, EShortcut::MOVE_RIGHT);

		if(abilitiesCount <= 6)
		{
			leftBtn->block(true);
			rightBtn->block(true);
		}

		pos.h += abilitiesBackground->pos.h;
	}
}

CStackWindow::MainSection::MainSection(CStackWindow * owner, int yOffset, bool showExp, bool showArt)
	: CWindowSection(owner, getBackgroundName(showExp, showArt), yOffset)
{
	OBJECT_CONSTRUCTION;

	statNames =
	{
		LIBRARY->generaltexth->primarySkillNames[0], //ATTACK
		LIBRARY->generaltexth->primarySkillNames[1],//DEFENCE
		LIBRARY->generaltexth->allTexts[198],//SHOTS
		LIBRARY->generaltexth->allTexts[199],//DAMAGE

		LIBRARY->generaltexth->allTexts[388],//HEALTH
		LIBRARY->generaltexth->allTexts[200],//HEALTH_LEFT
		LIBRARY->generaltexth->zelp[441].first,//SPEED
		LIBRARY->generaltexth->allTexts[399]//MANA
	};

	statFormats =
	{
		"%d (%d)",
		"%d (%d)",
		"%d (%d)",
		"%d - %d",

		"%d (%d)",
		"%d (%d)",
		"%d (%d)",
		"%d (%d)"
	};

	animation = std::make_shared<CCreaturePic>(5, 41, parent->info->creature);
	animationArea = std::make_shared<LRClickableArea>(Rect(5, 41, 100, 130), nullptr, [&]{
		if(!parent->info->creature->getDescriptionTranslated().empty())
			CRClickPopup::createAndPush(parent->info->creature->getDescriptionTranslated());
	});


	if(parent->info->stackNode != nullptr && parent->info->commander == nullptr)
	{
		//normal stack, not a commander and not non-existing stack (e.g. recruitment dialog)
		animation->setAmount(parent->info->creatureCount);
	}

	name = std::make_shared<CLabel>(215, 13, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, parent->info->getName());

	const CStack* battleStack = parent->info->stack;

	int dmgMultiply = 1;
	if (battleStack != nullptr && battleStack->hasBonusOfType(BonusType::SIEGE_WEAPON))
	{
		static const auto bonusSelector =
			Selector::sourceTypeSel(BonusSource::ARTIFACT).Or(
			Selector::sourceTypeSel(BonusSource::HERO_BASE_SKILL)).And(
			Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::ATTACK)));

		dmgMultiply += battleStack->valOfBonuses(bonusSelector);
	}
		
	icons = std::make_shared<CPicture>(ImagePath::builtin("stackWindow/icons"), 117, 32);

	morale = std::make_shared<MoraleLuckBox>(true, Rect(Point(321, 32), Point(42, 42) ));
	luck = std::make_shared<MoraleLuckBox>(false,  Rect(Point(375, 32), Point(42, 42) ));

	if(battleStack != nullptr) // in battle
	{
		addStatLabel(EStat::ATTACK, parent->info->creature->getAttack(battleStack->isShooter()), battleStack->getAttack(battleStack->isShooter()));
		addStatLabel(EStat::DEFENCE, parent->info->creature->getDefense(battleStack->isShooter()), battleStack->getDefense(battleStack->isShooter()));
		addStatLabel(EStat::DAMAGE, parent->info->stackNode->getMinDamage(battleStack->isShooter()) * dmgMultiply, battleStack->getMaxDamage(battleStack->isShooter()) * dmgMultiply);
		addStatLabel(EStat::HEALTH, parent->info->creature->getMaxHealth(), battleStack->getMaxHealth());
		addStatLabel(EStat::SPEED, parent->info->creature->getMovementRange(), battleStack->getMovementRange());

		if(battleStack->isShooter())
			addStatLabel(EStat::SHOTS, battleStack->shots.total(), battleStack->shots.available());
		if(battleStack->isCaster())
			addStatLabel(EStat::MANA, battleStack->casts.total(), battleStack->casts.available());
		addStatLabel(EStat::HEALTH_LEFT, battleStack->getFirstHPleft());

		morale->set(battleStack);
		luck->set(battleStack);
	}
	else
	{
		const bool shooter = parent->info->stackNode->hasBonusOfType(BonusType::SHOOTER) && parent->info->stackNode->valOfBonuses(BonusType::SHOTS);
		const bool caster = parent->info->stackNode->valOfBonuses(BonusType::CASTS);

		addStatLabel(EStat::ATTACK, parent->info->creature->getAttack(shooter), parent->info->stackNode->getAttack(shooter));
		addStatLabel(EStat::DEFENCE, parent->info->creature->getDefense(shooter), parent->info->stackNode->getDefense(shooter));
		addStatLabel(EStat::DAMAGE, parent->info->stackNode->getMinDamage(shooter), parent->info->stackNode->getMaxDamage(shooter));
		addStatLabel(EStat::HEALTH, parent->info->creature->getMaxHealth(), parent->info->stackNode->getMaxHealth());
		addStatLabel(EStat::SPEED, parent->info->creature->getMovementRange(), parent->info->stackNode->getMovementRange());

		if(shooter)
			addStatLabel(EStat::SHOTS, parent->info->stackNode->valOfBonuses(BonusType::SHOTS));
		if(caster)
			addStatLabel(EStat::MANA, parent->info->stackNode->valOfBonuses(BonusType::CASTS));

		morale->set(parent->info->stackNode);
		luck->set(parent->info->stackNode);
	}

	if(showExp)
	{
		const CStackInstance * stack = parent->info->stackNode;
		Point pos = showArt ? Point(321, 111) : Point(349, 111);
		if(parent->info->commander)
		{
			const CCommanderInstance * commander = parent->info->commander;
			expRankIcon = std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL42"), 4, 0, pos.x, pos.y);

			auto area = std::make_shared<LRClickableAreaWTextComp>(Rect(pos.x, pos.y, 44, 44), ComponentType::EXPERIENCE);
			expArea = area;
			area->text = LIBRARY->generaltexth->allTexts[2];
			area->component.value = commander->getExpRank();
			boost::replace_first(area->text, "%d", std::to_string(commander->getExpRank()));
			boost::replace_first(area->text, "%d", std::to_string(LIBRARY->heroh->reqExp(commander->getExpRank() + 1)));
			boost::replace_first(area->text, "%d", std::to_string(commander->getAverageExperience()));
		}
		else
		{
			expRankIcon = std::make_shared<CAnimImage>(AnimationPath::builtin("stackWindow/levels"), stack->getExpRank(), 0, pos.x, pos.y - 2);
			expArea = std::make_shared<LRClickableAreaWText>(Rect(pos.x, pos.y, 44, 44));
			expArea->text = parent->generateStackExpDescription();
		}
		expLabel = std::make_shared<CLabel>(
				pos.x + 21, pos.y + 55, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE,
				TextOperations::formatMetric(stack->getAverageExperience(), 6));
	}

	if(showArt)
	{
		Point pos = showExp ? Point(373, 109) : Point(347, 109);
		// ALARMA: do not refactor this into a separate function
		// otherwise, artifact icon is drawn near the hero's portrait
		// this is really strange
		auto art = parent->info->stackNode->getArt(ArtifactPosition::CREATURE_SLOT);
		if(art)
		{
			parent->stackArtifact = std::make_shared<CArtPlace>(pos, art->getTypeId());
			parent->stackArtifact->setShowPopupCallback([](CComponentHolder & artPlace, const Point & cursorPosition)
				{
					artPlace.LRClickableAreaWTextComp::showPopupWindow(cursorPosition);
				});
			if(parent->info->owner)
			{
				parent->stackArtifactButton = std::make_shared<CButton>(
						Point(pos.x , pos.y + 47), AnimationPath::builtin("stackWindow/cancelButton"),
						CButton::tooltipLocalized("vcmi.creatureWindow.returnArtifact"),	[this]()
				{
					parent->removeStackArtifact(ArtifactPosition::CREATURE_SLOT);
				});
			}
		}
	}
}


ImagePath CStackWindow::MainSection::getBackgroundName(bool showExp, bool showArt)
{
	if(showExp && showArt)
		return ImagePath::builtin("stackWindow/info-panel-2");
	else if(showExp || showArt)
		return ImagePath::builtin("stackWindow/info-panel-1");
	else
		return ImagePath::builtin("stackWindow/info-panel-0");
}

void CStackWindow::MainSection::addStatLabel(EStat index, int64_t value1, int64_t value2)
{
	const auto title = statNames.at(static_cast<size_t>(index));
	stats.push_back(std::make_shared<CLabel>(145, 32 + (int)index*19, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, title));

	const bool useRange = value1 != value2;
	std::string formatStr = useRange ? statFormats.at(static_cast<size_t>(index)) : "%d";

	boost::format fmt(formatStr);
	fmt % value1;
	if(useRange)
		fmt % value2;

	stats.push_back(std::make_shared<CLabel>(307, 48 + (int)index*19, FONT_SMALL, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, fmt.str()));
}

void CStackWindow::MainSection::addStatLabel(EStat index, int64_t value)
{
	addStatLabel(index, value, value);
}

CStackWindow::CStackWindow(const CStack * stack, bool popup)
	: CWindowObject(BORDERED | (popup ? RCLICK_POPUP : 0)),
	info(std::make_unique<UnitView>())
{
	info->stack = stack;
	info->stackNode = stack->base;
	info->commander = dynamic_cast<const CCommanderInstance*>(stack->base);
	info->creature = stack->unitType();
	info->creatureCount = stack->getCount();
	info->popupWindow = popup;
	init();
}

CStackWindow::CStackWindow(const CCreature * creature, bool popup)
	: CWindowObject(BORDERED | (popup ? RCLICK_POPUP : 0)),
	info(std::make_unique<UnitView>())
{
	info->creature = creature;
	info->popupWindow = popup;
	init();
}

CStackWindow::CStackWindow(const CStackInstance * stack, bool popup)
	: CWindowObject(BORDERED | (popup ? RCLICK_POPUP : 0)),
	info(std::make_unique<UnitView>())
{
	info->stackNode = stack;
	info->creature = stack->getCreature();
	info->creatureCount = stack->getCount();
	info->popupWindow = popup;
	info->owner = dynamic_cast<const CGHeroInstance *> (stack->getArmy());
	init();
}

CStackWindow::CStackWindow(const CStackInstance * stack, std::function<void()> dismiss, const UpgradeInfo & upgradeInfo, std::function<void(CreatureID)> callback)
	: CWindowObject(BORDERED),
	info(std::make_unique<UnitView>())
{
	info->stackNode = stack;
	info->creature = stack->getCreature();
	info->creatureCount = stack->getCount();

	if(upgradeInfo.canUpgrade())
	{
		info->upgradeInfo = std::make_optional(UnitView::StackUpgradeInfo(upgradeInfo));
		info->upgradeInfo->callback = callback;
	}
	
	info->dismissInfo = std::make_optional(UnitView::StackDismissInfo());
	info->dismissInfo->callback = dismiss;
	info->owner = dynamic_cast<const CGHeroInstance *> (stack->getArmy());
	init();
}

CStackWindow::CStackWindow(const CCommanderInstance * commander, bool popup)
	: CWindowObject(BORDERED | (popup ? RCLICK_POPUP : 0)),
	info(std::make_unique<UnitView>())
{
	info->stackNode = commander;
	info->creature = commander->getCreature();
	info->commander = commander;
	info->creatureCount = 1;
	info->popupWindow = popup;
	info->owner = dynamic_cast<const CGHeroInstance *> (commander->getArmy());
	init();
}

CStackWindow::CStackWindow(const CCommanderInstance * commander, std::vector<ui32> &skills, std::function<void(ui32)> callback)
	: CWindowObject(BORDERED),
	info(std::make_unique<UnitView>())
{
	info->stackNode = commander;
	info->creature = commander->getCreature();
	info->commander = commander;
	info->creatureCount = 1;
	info->levelupInfo = std::make_optional(UnitView::CommanderLevelInfo());
	info->levelupInfo->skills = skills;
	info->levelupInfo->callback = callback;
	info->owner = dynamic_cast<const CGHeroInstance *> (commander->getArmy());
	init();
}

CStackWindow::~CStackWindow()
{
	if(info->levelupInfo && !info->levelupInfo->skills.empty())
		info->levelupInfo->callback(vstd::find_pos(info->levelupInfo->skills, selectedSkill));
}

void CStackWindow::init()
{
	OBJECT_CONSTRUCTION;

	background = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), pos);

	if(!info->stackNode)
	{
		//does not contain any propagated bonuses
		fakeNode = std::make_unique<CStackInstance>(nullptr, info->creature->getId(), 1, true);
		info->stackNode = fakeNode.get();
	}

	selectedIcon = nullptr;
	selectedSkill = -1;
	if(info->levelupInfo && !info->levelupInfo->skills.empty())
		selectedSkill = info->levelupInfo->skills.front();

	activeTab = 0;

	initBonusesList();
	initSections();

	background->pos = pos;
}

void CStackWindow::initBonusesList()
{
	const IBonusBearer * bonusSource = (info->stack && !info->stack->base) 
    ? static_cast<const IBonusBearer*>(info->stack)  // Use CStack for war machines
    : static_cast<const IBonusBearer*>(info->stackNode);  // Use CStackInstance for regular units

	auto bonusToString = [bonusSource](const std::shared_ptr<Bonus> & bonus) -> std::string
	{
		if(!bonus->description.empty())
			return bonus->description.toString();
		else
			return LIBRARY->getBth()->bonusToString(bonus, bonusSource);
	};

	BonusList receivedBonuses = *bonusSource->getBonuses(CSelector(Bonus::Permanent));
	BonusList abilities = info->creature->getExportedBonusList();

	// remove all bonuses that are not propagated away
	// such bonuses should be present in received bonuses, and if not - this means that they are behind inactive limiter, such as inactive stack experience bonuses
	abilities.remove_if([](const Bonus* b){ return b->propagator == nullptr;});

	const auto & bonusSortingPredicate = [bonusToString](const std::shared_ptr<Bonus> & v1, const std::shared_ptr<Bonus> & v2){
		if (v1->source != v2->source)
		{
			int priorityV1 = v1->source == BonusSource::CREATURE_ABILITY ? -1 : static_cast<int>(v1->source);
			int priorityV2 = v2->source == BonusSource::CREATURE_ABILITY ? -1 : static_cast<int>(v2->source);
			return priorityV1 < priorityV2;
		}
		else
			return bonusToString(v1) < bonusToString(v2);
	};

	receivedBonuses.remove_if([](const Bonus* b)
	{
		return !LIBRARY->bth->shouldPropagateDescription(b->type);
	});

	std::vector<BonusList> groupedBonuses;
	while (!receivedBonuses.empty())
	{
		auto currentBonus = receivedBonuses.front();

		const auto & sameBonusPredicate = [currentBonus](const std::shared_ptr<Bonus> & b)
		{
			return currentBonus->type == b->type && currentBonus->subtype == b->subtype;
		};

		groupedBonuses.emplace_back();

		std::copy_if(receivedBonuses.begin(), receivedBonuses.end(), std::back_inserter(groupedBonuses.back()), sameBonusPredicate);
		receivedBonuses.remove_if(Selector::typeSubtype(currentBonus->type, currentBonus->subtype));
		// FIXME: potential edge case: unit has ability that is propagated away (and needs to be displayed), but also receives same bonus from someplace else
		abilities.remove_if(Selector::typeSubtype(currentBonus->type, currentBonus->subtype));
	}

	// Add any remaining abilities of this unit that don't affect it at the moment, such as abilities that are propagated away, e.g. to other side in combat
	BonusList visibleBonuses = abilities;

	for (auto & group : groupedBonuses)
	{
		// Try to find the bonus in the group that represents the final effect in the best way.
		std::sort(group.begin(), group.end(), bonusSortingPredicate);

		BonusList groupIndepMin = group;
		BonusList groupIndepMax = group;
		BonusList groupNoMinMax = group;
		BonusList groupBaseOnly = group;

		groupIndepMin.remove_if([](const Bonus * b) { return b->valType != BonusValueType::INDEPENDENT_MIN; });
		groupIndepMax.remove_if([](const Bonus * b) { return b->valType != BonusValueType::INDEPENDENT_MAX; });
		groupNoMinMax.remove_if([](const Bonus * b) { return b->valType == BonusValueType::INDEPENDENT_MAX || b->valType == BonusValueType::INDEPENDENT_MIN; });
		groupBaseOnly.remove_if([](const Bonus * b) { return b->valType != BonusValueType::ADDITIVE_VALUE || b->valType == BonusValueType::BASE_NUMBER; });

		int valIndepMin = groupIndepMin.totalValue();
		int valIndepMax = groupIndepMax.totalValue();
		int valNoMinMax = groupNoMinMax.totalValue();

		BonusList usedGroup;

		if (!groupIndepMin.empty() && valNoMinMax != valIndepMin)
			usedGroup = groupIndepMin; // bonus value was limited due to INDEPENDENT_MIN bonus -> show this bonus
		else if (!groupIndepMax.empty() && valNoMinMax != valIndepMax)
			usedGroup = groupIndepMax; // bonus value was limited due to INDEPENDENT_MAX bonus -> show this bonus
		else if (!groupBaseOnly.empty())
			usedGroup = groupNoMinMax; // bonus value is not limited and has bonuses other than percent to base / percent to all - show first non-independent bonus

		// It is possible that empty group was selected. For example, there is only INDEPENDENT effect with value of 0, which does not actually has any effect on this unit
		// For example, orb of vulnerability on unit without any resistances
		if (!usedGroup.empty())
			visibleBonuses.push_back(usedGroup.front());
	}

	std::sort(visibleBonuses.begin(), visibleBonuses.end(), bonusSortingPredicate);

	BonusInfo bonusInfo;
	for(auto b : visibleBonuses)
	{
		bonusInfo.description = bonusToString(b);
		//FIXME: we possibly use fakeNode, which does not have the correct bonus value
		bonusInfo.imagePath = info->stackNode->bonusToGraphics(b);
		bonusInfo.bonusSource = b->source;

		//if it's possible to give any description or image for this kind of bonus
		if(!bonusInfo.description.empty() && !b->hidden)
			activeBonuses.push_back(bonusInfo);
	}
}

void CStackWindow::initSections()
{
	OBJECT_CONSTRUCTION;

	bool showArt = GAME->interface() && GAME->interface()->cb->getSettings().getBoolean(EGameSettings::MODULE_STACK_ARTIFACT) && info->commander == nullptr && info->stackNode;
	bool showExp = ((GAME->interface() && GAME->interface()->cb->getSettings().getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE)) || info->commander != nullptr) && info->stackNode;

	mainSection = std::make_shared<MainSection>(this, pos.h, showExp, showArt);

	pos.w = mainSection->pos.w;
	pos.h += mainSection->pos.h;

	if(info->stack) // in battle
	{
		activeSpellsSection = std::make_shared<ActiveSpellsSection>(this, pos.h);
		pos.h += activeSpellsSection->pos.h;
	}

	if(info->commander)
	{
		auto onCreate = [this](size_t index) -> std::shared_ptr<CIntObject>
		{
			auto obj = switchTab(index);

			if(obj)
				obj->enable();
			return obj;
		};

		auto deactivateObj = [=](std::shared_ptr<CIntObject> obj)
		{
			obj->disable();
		};

		commanderMainSection = std::make_shared<CommanderMainSection>(this, 0);

		auto size = std::make_optional<size_t>((info->levelupInfo) ? 4 : 3);
		commanderBonusesSection = std::make_shared<BonusesSection>(this, 0, size);
		deactivateObj(commanderBonusesSection);

		commanderTab = std::make_shared<CTabbedInt>(onCreate, Point(0, pos.h), 0);

		pos.h += commanderMainSection->pos.h;
	}

	if(!info->commander && !activeBonuses.empty())
	{
		bonusesSection = std::make_shared<BonusesSection>(this, pos.h);
		pos.h += bonusesSection->pos.h;
	}

	if(!info->popupWindow)
	{
		buttonsSection = std::make_shared<ButtonsSection>(this, pos.h);
		pos.h += buttonsSection->pos.h;
		//FIXME: add status bar to image?
	}
	updateShadow();
	pos = center(pos);
}

std::string CStackWindow::generateStackExpDescription()
{
	const CStackInstance * stack = info->stackNode;
	const CCreature * creature = info->creature;

	int tier = stack->getType()->getLevel();
	int rank = stack->getExpRank();
	if (!vstd::iswithin(tier, 1, 7))
		tier = 0;
	int number;
	std::string expText = LIBRARY->generaltexth->translate("vcmi.stackExperience.description");
	boost::replace_first(expText, "%s", creature->getNamePluralTranslated());
	boost::replace_first(expText, "%s", LIBRARY->generaltexth->translate("vcmi.stackExperience.rank", rank));
	boost::replace_first(expText, "%i", std::to_string(rank));
	boost::replace_first(expText, "%i", std::to_string(stack->getAverageExperience()));
	number = static_cast<int>(LIBRARY->creh->expRanks[tier][rank] - stack->getAverageExperience());
	boost::replace_first(expText, "%i", std::to_string(number));

	number = LIBRARY->creh->maxExpPerBattle[tier]; //percent
	boost::replace_first(expText, "%i%", std::to_string(number));
	number *= LIBRARY->creh->expRanks[tier].back() / 100; //actual amount
	boost::replace_first(expText, "%i", std::to_string(number));

	boost::replace_first(expText, "%i", std::to_string(stack->getCount())); //Number of Creatures in stack

	int expmin = std::max(LIBRARY->creh->expRanks[tier][std::max(rank-1, 0)], (ui32)1);
	number = stack->getTotalExperience() / expmin - stack->getCount(); //Maximum New Recruits without losing current Rank
	boost::replace_first(expText, "%i", std::to_string(number)); //TODO

	boost::replace_first(expText, "%.2f", std::to_string(1)); //TODO Experience Multiplier
	number = LIBRARY->creh->expAfterUpgrade;
	boost::replace_first(expText, "%.2f", std::to_string(number) + "%"); //Upgrade Multiplier

	expmin = LIBRARY->creh->expRanks[tier][9];
	int expmax = LIBRARY->creh->expRanks[tier][10];
	number = expmax - expmin;
	boost::replace_first(expText, "%i", std::to_string(number)); //Experience after Rank 10
	number = (stack->getCount() * (expmax - expmin)) / expmin;
	boost::replace_first(expText, "%i", std::to_string(number)); //Maximum New Recruits to remain at Rank 10 if at Maximum Experience

	return expText;
}

std::string CStackWindow::getCommanderSkillDescription(int skillIndex, int skillLevel)
{
	constexpr std::array skillNames = {
		"attack",
		"defence",
		"health",
		"damage",
		"speed",
		"magic"
	};

	std::string textID = TextIdentifier("vcmi", "commander", "skill", skillNames.at(skillIndex), skillLevel).get();

	return LIBRARY->generaltexth->translate(textID);
}

void CStackWindow::setSelection(si32 newSkill, std::shared_ptr<CCommanderSkillIcon> newIcon)
{
	auto getSkillDescription = [this](int skillIndex, bool selected) -> std::string
	{
		if(selected)
			return getCommanderSkillDescription(skillIndex, info->commander->secondarySkills[skillIndex] + 1); //upgrade description
		else
			return getCommanderSkillDescription(skillIndex, info->commander->secondarySkills[skillIndex]);
	};

	auto getSkillImage = [this](int skillIndex)
	{
		bool selected = ((selectedSkill == skillIndex) && info->levelupInfo );
		return skillToFile(skillIndex, info->commander->secondarySkills[skillIndex], selected);
	};

	OBJECT_CONSTRUCTION;
	int oldSelection = selectedSkill; // update selection
	selectedSkill = newSkill;

	if(selectedIcon && oldSelection < 100) // recreate image on old selection, only for skills
		selectedIcon->setObject(std::make_shared<CPicture>(getSkillImage(oldSelection)));

	if(selectedIcon)
	{
		if(!selectedIcon->getIsMasterAbility()) //unlike WoG, in VCMI master skill descriptions are taken from bonus descriptions
		{
			selectedIcon->text = getSkillDescription(oldSelection, false); //update previously selected icon's message to existing skill level
		}
		selectedIcon->deselect();
	}

	selectedIcon = newIcon; // update new selection
	if(newSkill < 100)
	{
		newIcon->setObject(std::make_shared<CPicture>(getSkillImage(newSkill)));
		if(!newIcon->getIsMasterAbility())
		{
			newIcon->text = getSkillDescription(newSkill, true); //update currently selected icon's message to show upgrade description
		}
	}
}

std::shared_ptr<CIntObject> CStackWindow::switchTab(size_t index)
{
	std::shared_ptr<CIntObject> ret;
	switch(index)
	{
	case 0:
		{
			activeTab = 0;
			ret = commanderMainSection;
		}
		break;
	case 1:
		{
			activeTab = 1;
			ret = commanderBonusesSection;
		}
		break;
	default:
		break;
	}

	return ret;
}

void CStackWindow::removeStackArtifact(ArtifactPosition pos)
{
	auto art = info->stackNode->getArt(ArtifactPosition::CREATURE_SLOT);
	if(!art)
	{
		logGlobal->error("Attempt to remove missing artifact");
		return;
	}
	const auto slot = ArtifactUtils::getArtBackpackPosition(info->owner, art->getTypeId());
	if(slot != ArtifactPosition::PRE_FIRST)
	{
		auto artLoc = ArtifactLocation(info->owner->id, pos);
		artLoc.creature = info->stackNode->getArmy()->findStack(info->stackNode);
		GAME->interface()->cb->swapArtifacts(artLoc, ArtifactLocation(info->owner->id, slot));
		stackArtifactButton.reset();
		stackArtifact.reset();
		redraw();
	}
}
