/*
 * EditorObjectDialogs.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorObjectDialogs.h"

#include "../EditorMap.h"
#include "../EditorTexts.h"

#include "../../GameEngine.h"
#include "../../gui/WindowHandler.h"
#include "../../widgets/CCheckBox.h"
#include "../../windows/CChoiceDialog.h"
#include "../../widgets/CDropDown.h"
#include "../../widgets/CFormView.h"
#include "../../widgets/CItemValuePicker.h"
#include "../../widgets/CMultiLineEdit.h"
#include "../../widgets/CNumberInput.h"
#include "../../widgets/CPlayerSetEditor.h"
#include "../../widgets/CSelectList.h"
#include "../../widgets/CTabBar.h"
#include "../../widgets/CTextButton.h"
#include "../../widgets/CTextField.h"
#include "../../widgets/Images.h"
#include "../../widgets/TextControls.h"
#include "../../widgets/UiTheme.h"

#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/CSkillHandler.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/constants/StringConstants.h"
#include "../../../lib/entities/artifact/ArtifactUtils.h"
#include "../../../lib/entities/artifact/CArtHandler.h"
#include "../../../lib/entities/hero/CHeroHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/army/CArmedInstance.h"
#include "../../../lib/mapObjects/army/CStackInstance.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/spells/CSpellHandler.h"

#include <boost/algorithm/string/predicate.hpp>

namespace
{
constexpr int LABEL_WIDTH = 140;

std::vector<std::string> skillLevelNames()
{
	return {EditorTexts::get("level.basic"), EditorTexts::get("level.advanced"), EditorTexts::get("level.expert")};
}
}

// ---------------------------------------------------------------------------------------------

EditorArmyDialog::EditorArmyDialog(CArmedInstance & army, const std::function<void()> & onChanged)
	: CDialogWindow(Point(560, 360), EditorTexts::get("army.title"))
	, army(army)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();

	std::vector<std::string> names{EditorTexts::get("army.empty")};
	for(const auto & creature : LIBRARY->creh->objects)
		names.push_back(creature->getNamePluralTranslated());

	const int countWidth = 130;
	for(int i = 0; i < SLOTS; ++i)
	{
		const int y = i * (UiTheme::CONTROL_HEIGHT + 4);
		const bool occupied = army.hasStackAtSlot(SlotID(i));

		creatures[i] = std::make_shared<CDropDown>(Rect(0, y, size.x - countWidth - UiTheme::SPACING, UiTheme::CONTROL_HEIGHT), names,
			occupied ? army.getCreature(SlotID(i))->getIndex() + 1 : 0, nullptr, EditorTexts::get("army.creature"));
		counts[i] = std::make_shared<CNumberInput>(Rect(size.x - countWidth, y, countWidth, UiTheme::CONTROL_HEIGHT), occupied ? army.getStackCount(SlotID(i)) : 0, 0, 999999);
	}

	formation = std::make_shared<CDropDown>(Rect(0, SLOTS * (UiTheme::CONTROL_HEIGHT + 4) + 6, size.x, UiTheme::CONTROL_HEIGHT),
		std::vector<std::string>{EditorTexts::get("army.tight"), EditorTexts::get("army.loose")}, army.formation == EArmyFormation::TIGHT ? 0 : 1, nullptr, EditorTexts::get("army.formation"));

	onAccept = [this, onChanged]()
	{
		commit();
		if(onChanged)
			onChanged();
		return true;
	};
}

void EditorArmyDialog::commit()
{
	for(int i = 0; i < SLOTS; ++i)
	{
		const int selected = creatures[i]->getSelected();
		if(selected <= 0)
		{
			if(army.hasStackAtSlot(SlotID(i)))
				army.eraseStack(SlotID(i));
			continue;
		}

		const CreatureID creatureId(selected - 1);
		const int amount = counts[i]->getValue();

		if(amount)
		{
			army.setCreature(SlotID(i), creatureId, amount);
		}
		else
		{
			if(army.hasStackAtSlot(SlotID(i)))
				army.eraseStack(SlotID(i));
			army.putStack(SlotID(i), std::make_unique<CStackInstance>(army.cb, creatureId, amount, false));
		}
	}

	army.setFormation(formation->getSelected() == 0 ? EArmyFormation::TIGHT : EArmyFormation::LOOSE);
}

// ---------------------------------------------------------------------------------------------

EditorMessageDialog::EditorMessageDialog(const std::string & title, const std::string & text, const std::function<void(const std::string &)> & onSaved)
	: CDialogWindow(Point(560, 360), title)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	edit = std::make_shared<CMultiLineEdit>(Rect(0, 0, size.x, size.y), text);

	onAccept = [this, onSaved]()
	{
		if(onSaved)
			onSaved(edit->getText());
		return true;
	};
}

// ---------------------------------------------------------------------------------------------

EditorPlayerSetDialog::EditorPlayerSetDialog(const std::string & title, const std::set<PlayerColor> & selected, const std::function<void(const std::set<PlayerColor> &)> & onSaved)
	: CDialogWindow(Point(360, 300), title)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	players = std::make_shared<CPlayerSetEditor>(Rect(0, 0, size.x, CPlayerSetEditor::getHeight(2)), selected, nullptr, 2);

	onAccept = [this, onSaved]()
	{
		if(onSaved)
			onSaved(players->getValue());
		return true;
	};
}

// ---------------------------------------------------------------------------------------------

EditorPortraitDialog::EditorPortraitDialog(CGHeroInstance & hero, const std::function<void()> & onChanged)
	: CDialogWindow(Point(320, 300), EditorTexts::get("portrait.title"))
	, hero(hero)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();

	const auto portraitSource = hero.getPortraitSource();
	bool isDefault = true;
	if(portraitSource.isValid())
	{
		portraitIndex = LIBRARY->heroh->getById(portraitSource)->getIndex();
		if(hero.customPortraitSource.isValid())
			isDefault = false;
	}

	const int buttonWidth = 40;
	defaultCheck = std::make_shared<CCheckBox>(Rect(0, 0, size.x, UiTheme::CONTROL_HEIGHT), EditorTexts::get("portrait.default"), isDefault, [this](bool on)
	{
		if(on)
			portraitIndex = LIBRARY->heroh->getById(this->hero.getHeroTypeID())->getIndex();

		drawPortrait();
	});

	auto previous = std::make_shared<CTextButton>(Rect(0, size.y - UiTheme::CONTROL_HEIGHT, buttonWidth, UiTheme::CONTROL_HEIGHT), "<", [this]()
	{
		if(defaultCheck->isChecked())
			return;

		portraitIndex = portraitIndex > 0 ? portraitIndex - 1 : static_cast<int>(LIBRARY->heroh->objects.size()) - 1;
		drawPortrait();
	});
	auto next = std::make_shared<CTextButton>(Rect(size.x - buttonWidth, size.y - UiTheme::CONTROL_HEIGHT, buttonWidth, UiTheme::CONTROL_HEIGHT), ">", [this]()
	{
		if(defaultCheck->isChecked())
			return;

		portraitIndex = portraitIndex < static_cast<int>(LIBRARY->heroh->objects.size()) - 1 ? portraitIndex + 1 : 0;
		drawPortrait();
	});
	navButtons = {previous, next};

	drawPortrait();

	onAccept = [this, onChanged]()
	{
		commit();
		if(onChanged)
			onChanged();
		return true;
	};
}

void EditorPortraitDialog::drawPortrait()
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	portrait.reset();
	const Point size = contentSize();
	const int icon = LIBRARY->heroh->getByIndex(portraitIndex)->getIconIndex();
	portrait = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), icon, Rect((size.x - 120) / 2, UiTheme::CONTROL_HEIGHT + 10, 120, 132));
}

void EditorPortraitDialog::commit()
{
	if(portraitIndex == LIBRARY->heroh->getById(hero.getHeroTypeID())->getIndex())
		hero.customPortraitSource = HeroTypeID::NONE;
	else
		hero.customPortraitSource = LIBRARY->heroh->getByIndex(portraitIndex)->getId();
}

// ---------------------------------------------------------------------------------------------

EditorHeroSkillsDialog::EditorHeroSkillsDialog(CGHeroInstance & hero, const std::function<void()> & onChanged)
	: CDialogWindow(Point(520, 420), EditorTexts::get("heroSkills.title"))
	, hero(hero)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), LABEL_WIDTH);

	const std::array<PrimarySkill, 4> skills{PrimarySkill::ATTACK, PrimarySkill::DEFENSE, PrimarySkill::SPELL_POWER, PrimarySkill::KNOWLEDGE};
	for(size_t i = 0; i < skills.size(); ++i)
		primary[i] = form->addNumber(EditorTexts::get(std::string("heroSkills.") + NPrimarySkill::names[i]), hero.getBasePrimarySkillValue(skills[i]), 0, 99, nullptr);

	ItemValueConfig config;
	config.title = EditorTexts::get("heroSkills.secondary");
	for(const auto & skill : LIBRARY->skillh->objects)
		config.itemNames.push_back(skill->getNameTranslated());
	config.minValue = 1;
	config.maxValue = 3;
	config.valueNames = skillLevelNames();

	const bool customized = hero.secSkills.empty() || hero.secSkills.front().first.getNum() != -1;
	ItemValueEntries entries;
	if(customized)
		for(const auto & skill : hero.secSkills)
			entries.emplace_back(skill.first.getNum(), skill.second);

	customizeCheck = form->addCheck(EditorTexts::get("heroSkills.customize"), customized, [this](bool on)
	{
		secondary->setInputEnabled(on);
	});
	secondary = std::make_shared<CItemValuePicker>(Rect(0, 0, form->contentWidth() - LABEL_WIDTH, UiTheme::CONTROL_HEIGHT), config, entries);
	form->addWidget(EditorTexts::get("heroSkills.secondary"), secondary);
	form->finish();

	onAccept = [this, onChanged]()
	{
		commit();
		if(onChanged)
			onChanged();
		return true;
	};
}

void EditorHeroSkillsDialog::commit()
{
	const std::array<PrimarySkill, 4> skills{PrimarySkill::ATTACK, PrimarySkill::DEFENSE, PrimarySkill::SPELL_POWER, PrimarySkill::KNOWLEDGE};
	for(size_t i = 0; i < skills.size(); ++i)
		hero.pushPrimSkill(skills[i], primary[i]->getValue());

	hero.secSkills.clear();

	if(!customizeCheck->isChecked())
	{
		hero.secSkills.push_back(std::make_pair(SecondarySkill(-1), -1));
		return;
	}

	for(const auto & entry : secondary->getEntries())
		hero.secSkills.push_back(std::make_pair(SecondarySkill(entry.first), entry.second));
}

// ---------------------------------------------------------------------------------------------

EditorHeroSpellsDialog::EditorHeroSpellsDialog(CGHeroInstance & hero, const std::function<void()> & onChanged)
	: CDialogWindow(Point(480, 520), EditorTexts::get("heroSpells.title"))
	, hero(hero)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();

	for(auto const & spellID : LIBRARY->spellh->getDefaultAllowed())
	{
		const auto * spell = spellID.toEntity(LIBRARY);
		const int spellLevel = spell->getLevel();
		if(spellLevel >= 1 && spellLevel <= GameConstants::SPELL_LEVELS)
			levels[spellLevel - 1].push_back({spellID.getNum(), spell->getNameTranslated(), hero.spellbookContainsSpell(spellID)});
	}

	const int rowHeight = UiTheme::CONTROL_HEIGHT;
	customizeCheck = std::make_shared<CCheckBox>(Rect(0, 0, size.x, rowHeight), EditorTexts::get("heroSpells.customize"), hero.spellbookContainsSpell(SpellID::PRESET), [this](bool)
	{
		refreshList();
	});

	filterField = std::make_shared<CTextField>(Rect(0, rowHeight + UiTheme::SPACING, size.x, rowHeight), "", [this](const std::string &) { refreshList(); });

	std::vector<std::string> tabNames;
	for(int i = 0; i < GameConstants::SPELL_LEVELS; ++i)
		tabNames.push_back(EditorTexts::format("heroSpells.level", {std::to_string(i + 1)}));
	tabs = std::make_shared<CTabBar>(Rect(0, 2 * (rowHeight + UiTheme::SPACING), size.x, rowHeight), tabNames, 0, [this](int index)
	{
		level = index;
		refreshList();
	});

	const int listTop = 3 * (rowHeight + UiTheme::SPACING);
	list = std::make_shared<CSelectList>(Rect(0, listTop, size.x, size.y - listTop), CSelectList::Mode::CHECK, UiTheme::ROW_HEIGHT);
	list->onCheck = [this](int row, bool on)
	{
		levels[level].at(shown.at(row)).known = on;
	};

	refreshList();

	onAccept = [this, onChanged]()
	{
		commit();
		if(onChanged)
			onChanged();
		return true;
	};
}

void EditorHeroSpellsDialog::refreshList()
{
	const std::string filter = filterField->getText();

	shown.clear();
	std::vector<CSelectList::Row> rows;
	for(size_t i = 0; i < levels[level].size(); ++i)
	{
		const auto & entry = levels[level][i];
		if(!filter.empty() && !boost::algorithm::icontains(entry.name, filter))
			continue;

		CSelectList::Row row;
		row.text = entry.name;
		row.checked = entry.known;
		row.enabled = customizeCheck->isChecked();
		rows.push_back(std::move(row));
		shown.push_back(static_cast<int>(i));
	}
	list->setRows(std::move(rows));
}

void EditorHeroSpellsDialog::commit()
{
	if(!customizeCheck->isChecked())
	{
		hero.removeAllSpells();
		return;
	}

	hero.addSpellToSpellbook(SpellID::PRESET);
	for(const auto & spells : levels)
	{
		for(const auto & entry : spells)
		{
			if(entry.known)
				hero.addSpellToSpellbook(SpellID(entry.id));
			else
				hero.removeSpellFromSpellbook(SpellID(entry.id));
		}
	}
}

// ---------------------------------------------------------------------------------------------

EditorHeroArtifactsDialog::EditorHeroArtifactsDialog(EditorMap & editor, CGHeroInstance & hero, const std::function<void()> & onChanged)
	: CDialogWindow(Point(520, 480), EditorTexts::get("heroArtifacts.title"))
	, editor(editor)
	, hero(hero)
	, fittingSet(hero)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	const int buttonHeight = 24;
	const int buttonWidth = (size.x - UiTheme::SPACING) / 2;

	list = std::make_shared<CSelectList>(Rect(0, 0, size.x, size.y - buttonHeight - UiTheme::SPACING), CSelectList::Mode::SINGLE, UiTheme::ROW_HEIGHT);
	addButtonControl = std::make_shared<CTextButton>(Rect(0, size.y - buttonHeight, buttonWidth, buttonHeight), EditorTexts::get("heroArtifacts.add"), [this]() { addArtifact(); });
	removeButtonControl = std::make_shared<CTextButton>(Rect(buttonWidth + UiTheme::SPACING, size.y - buttonHeight, buttonWidth, buttonHeight), EditorTexts::get("heroArtifacts.remove"), [this]() { removeSelected(); });

	for(const auto & [position, slotInfo] : fittingSet.artifactsWorn)
		rows.push_back({LIBRARY->arth->getById(slotInfo.getArt()->getTypeId())->getIndex(), position});
	for(const auto & art : hero.artifactsInBackpack)
		rows.push_back({LIBRARY->arth->getById(art.getArt()->getTypeId())->getIndex(), ArtifactPosition::BACKPACK_START});

	refreshRows();

	onAccept = [this, onChanged]()
	{
		commit();
		if(onChanged)
			onChanged();
		return true;
	};
}

void EditorHeroArtifactsDialog::refreshRows()
{
	std::vector<CSelectList::Row> listRows;
	for(const auto & row : rows)
	{
		CSelectList::Row item;
		item.text = LIBRARY->arth->getByIndex(row.artifactIndex)->getNameTranslated();
		item.detail = ArtifactUtils::isSlotBackpack(row.slot) ? NArtifactPosition::backpack : NArtifactPosition::namesHero[row.slot.num];
		listRows.push_back(std::move(item));
	}
	list->setRows(std::move(listRows), true);
}

void EditorHeroArtifactsDialog::addArtifact()
{
	// positions the artifact can go to
	auto slots = std::make_shared<std::vector<ArtifactPosition>>();
	std::vector<std::string> slotNames;
	for(const auto & slot : ArtifactUtils::allWornSlots())
	{
		if(fittingSet.isPositionFree(slot))
		{
			slots->push_back(slot);
			slotNames.push_back(NArtifactPosition::namesHero[slot.num]);
		}
	}
	slots->push_back(ArtifactPosition::BACKPACK_START);
	slotNames.push_back(NArtifactPosition::backpack);

	CChoiceDialog::chooseOne(EditorTexts::get("heroArtifacts.slot"), CChoiceDialog::makeItems(slotNames), 0, [this, slots](int slotIndex)
	{
		const ArtifactPosition slot = slots->at(slotIndex);

		auto indexes = std::make_shared<std::vector<int>>();
		std::vector<std::string> names;
		for(const auto & art : LIBRARY->arth->getDefaultAllowed())
		{
			auto artifact = art.toArtifact();

			// spell scroll is not available because it needs a spell
			if(artifact->canBePutAt(&fittingSet, slot, true) && artifact->getId() != ArtifactID::SPELL_SCROLL)
			{
				indexes->push_back(artifact->getIndex());
				names.push_back(artifact->getNameTranslated());
			}
		}

		CChoiceDialog::chooseOne(EditorTexts::get("heroArtifacts.artifact"), CChoiceDialog::makeItems(names), 0, [this, slot, indexes](int artifactChoice)
		{
			const int artifactIndex = indexes->at(artifactChoice);
			auto artifact = this->editor.getMap()->createArtifact(LIBRARY->arth->getByIndex(artifactIndex)->getId());
			fittingSet.putArtifact(slot, artifact);
			rows.push_back({artifactIndex, slot});
			refreshRows();
		});
	});
}

void EditorHeroArtifactsDialog::removeSelected()
{
	const int index = list->getSelected();
	if(index < 0 || index >= static_cast<int>(rows.size()))
		return;

	const Row row = rows[index];
	if(ArtifactUtils::isSlotBackpack(row.slot))
	{
		int backpackIndex = 0;
		for(int i = 0; i < index; ++i)
			if(ArtifactUtils::isSlotBackpack(rows[i].slot))
				++backpackIndex;

		fittingSet.removeArtifact(ArtifactPosition::BACKPACK_START + backpackIndex);
	}
	else
		fittingSet.removeArtifact(row.slot);

	rows.erase(rows.begin() + index);
	refreshRows();
}

void EditorHeroArtifactsDialog::commit()
{
	while(!hero.artifactsWorn.empty())
		hero.removeArtifact(hero.artifactsWorn.begin()->first);

	while(!hero.artifactsInBackpack.empty())
		hero.removeArtifact(ArtifactPosition::BACKPACK_START + static_cast<int>(hero.artifactsInBackpack.size()) - 1);

	for(const auto & [artPosition, artSlotInfo] : fittingSet.artifactsWorn)
		hero.putArtifact(artPosition, artSlotInfo.getArt());

	for(const auto & art : fittingSet.artifactsInBackpack)
		hero.putArtifact(ArtifactPosition::BACKPACK_START + static_cast<int>(hero.artifactsInBackpack.size()), art.getArt());
}
