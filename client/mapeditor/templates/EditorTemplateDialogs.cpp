/*
 * EditorTemplateDialogs.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorTemplateDialogs.h"

#include "../EditorTexts.h"

#include "../../GameEngine.h"
#include "../../gui/WindowHandler.h"
#include "../../widgets/CDropDown.h"
#include "../../widgets/CFormView.h"
#include "../../widgets/CListEditor.h"
#include "../../widgets/CNumberInput.h"
#include "../../widgets/CTabBar.h"
#include "../../widgets/CTextField.h"
#include "../../widgets/UiTheme.h"
#include "../../windows/CChoiceDialog.h"

#include "../../../lib/CSkillHandler.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/TerrainHandler.h"
#include "../../../lib/entities/ResourceTypeHandler.h"
#include "../../../lib/entities/artifact/CArtHandler.h"
#include "../../../lib/entities/faction/CTownHandler.h"
#include "../../../lib/entities/hero/CHeroHandler.h"
#include "../../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../../lib/mapObjects/CGObjectInstance.h"
#include "../../../lib/spells/CSpellHandler.h"
#include "../../../lib/texts/CGeneralTextHandler.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>

namespace
{
constexpr int LABEL_WIDTH = 150;
constexpr int MAX_MINES = 100;
constexpr int MAX_TREASURE_VALUE = 1000000;
constexpr int MAX_TREASURE_DENSITY = 1000;
constexpr int MAX_OBJECT_VALUE = 10000;
constexpr int MAX_OBJECT_PROBABILITY = 1000;
constexpr int MAX_OBJECTS_PER_ZONE = 100;
constexpr int MAX_REQUIRED_COUNT = 100;
constexpr int MAX_GUARD = 999999;

template<class Id, class Handler>
void editEntities(const std::string & title, const Handler & handler, std::set<Id> & target)
{
	std::vector<CChoiceDialog::Item> items;
	std::set<int> checked;

	for(const auto & entity : handler.objects)
	{
		CChoiceDialog::Item item;
		item.text = entity->getNameTranslated();
		items.push_back(std::move(item));

		if(vstd::contains(target, entity->getId()))
			checked.insert(static_cast<int>(items.size()) - 1);
	}

	CChoiceDialog::chooseMany(title, std::move(items), checked, [&target](const std::set<int> & chosen)
	{
		target.clear();
		for(int index : chosen)
			target.insert(Id(index));
	});
}

/// Window that edits the three numbers of a treasure group
class TreasureDialog : public CDialogWindow
{
	std::shared_ptr<CFormView> form;
	std::array<std::shared_ptr<CNumberInput>, 3> inputs;

public:
	TreasureDialog(const CTreasureInfo & info, const std::function<void(const CTreasureInfo &)> & onSaved)
		: CDialogWindow(Point(420, 220), EditorTexts::get("template.treasure"))
	{
		OBJECT_CONSTRUCTION_TARGETED(content());

		const Point size = contentSize();
		form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), LABEL_WIDTH);
		inputs[0] = form->addNumber(EditorTexts::get("template.treasureMin"), static_cast<int>(info.min), 0, MAX_TREASURE_VALUE, nullptr);
		inputs[1] = form->addNumber(EditorTexts::get("template.treasureMax"), static_cast<int>(info.max), 0, MAX_TREASURE_VALUE, nullptr);
		inputs[2] = form->addNumber(EditorTexts::get("template.treasureDensity"), info.density, 0, MAX_TREASURE_DENSITY, nullptr);
		form->finish();

		onAccept = [this, onSaved]()
		{
			onSaved(CTreasureInfo(inputs[0]->getValue(), inputs[1]->getValue(), inputs[2]->getValue()));
			return true;
		};
	}
};

enum class HintMode
{
	LIKE_ZONE,
	NOT_LIKE_ZONE,
	RELATED_TO_ZONE_TERRAIN
};

/// Text of a town hint, the zones are listed separated by commas
struct HintDraft
{
	HintMode mode = HintMode::LIKE_ZONE;
	std::string zones = "0";
};

std::string hintModeName(HintMode mode)
{
	switch(mode)
	{
		case HintMode::LIKE_ZONE:
			return EditorTexts::get("template.hintLike");
		case HintMode::NOT_LIKE_ZONE:
			return EditorTexts::get("template.hintNotLike");
		default:
			return EditorTexts::get("template.hintTerrain");
	}
}

/// Window that edits one town hint
class HintDialog : public CDialogWindow
{
	std::shared_ptr<CFormView> form;
	std::shared_ptr<CDropDown> modeChoice;
	std::shared_ptr<CTextField> zonesField;

public:
	HintDialog(const HintDraft & draft, const std::function<void(const HintDraft &)> & onSaved)
		: CDialogWindow(Point(480, 200), EditorTexts::get("template.townHint"))
	{
		OBJECT_CONSTRUCTION_TARGETED(content());

		const Point size = contentSize();
		form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), LABEL_WIDTH);
		modeChoice = form->addChoice(EditorTexts::get("template.hintType"), {hintModeName(HintMode::LIKE_ZONE), hintModeName(HintMode::NOT_LIKE_ZONE), hintModeName(HintMode::RELATED_TO_ZONE_TERRAIN)}, static_cast<int>(draft.mode), nullptr);
		zonesField = form->addText(EditorTexts::get("template.hintZones"), draft.zones, nullptr);
		form->finish();

		onAccept = [this, onSaved]()
		{
			HintDraft result;
			result.mode = static_cast<HintMode>(modeChoice->getSelected());
			result.zones = zonesField->getText();

			// the zones are numbers separated by commas
			if(result.zones.empty() || result.zones.find_first_not_of("0123456789,") != std::string::npos)
				return false;

			onSaved(result);
			return true;
		};
	}
};

std::vector<int> parseZones(const std::string & text)
{
	std::vector<std::string> parts;
	boost::split(parts, text, boost::is_any_of(","));

	std::vector<int> result;
	for(const auto & part : parts)
		if(!part.empty())
			result.push_back(std::stoi(part));

	return result;
}

std::string categoryName(ObjectConfig::EObjectCategory category)
{
	using Category = ObjectConfig::EObjectCategory;
	switch(category)
	{
		case Category::OTHER: return EditorTexts::get("template.categoryOther");
		case Category::ALL: return EditorTexts::get("template.categoryAll");
		case Category::NONE: return EditorTexts::get("template.categoryNone");
		case Category::CREATURE_BANK: return EditorTexts::get("template.categoryBank");
		case Category::BONUS: return EditorTexts::get("template.categoryBonus");
		case Category::DWELLING: return EditorTexts::get("template.categoryDwelling");
		case Category::RESOURCE: return EditorTexts::get("template.categoryResource");
		case Category::RESOURCE_GENERATOR: return EditorTexts::get("template.categoryGenerator");
		case Category::SPELL_SCROLL: return EditorTexts::get("template.categoryScroll");
		case Category::RANDOM_ARTIFACT: return EditorTexts::get("template.categoryArtifact");
		case Category::PANDORAS_BOX: return EditorTexts::get("template.categoryPandora");
		case Category::QUEST_ARTIFACT: return EditorTexts::get("template.categoryQuestArtifact");
		case Category::SEER_HUT: return EditorTexts::get("template.categorySeerHut");
	}
	return "";
}

const std::vector<ObjectConfig::EObjectCategory> & allCategories()
{
	using Category = ObjectConfig::EObjectCategory;
	static const std::vector<Category> categories{Category::OTHER, Category::ALL, Category::NONE, Category::CREATURE_BANK, Category::BONUS, Category::DWELLING, Category::RESOURCE,
		Category::RESOURCE_GENERATOR, Category::SPELL_SCROLL, Category::RANDOM_ARTIFACT, Category::PANDORAS_BOX, Category::QUEST_ARTIFACT, Category::SEER_HUT};
	return categories;
}

/// Window that edits a few numbers and one object of the list
class ObjectRowDialog : public CDialogWindow
{
	std::shared_ptr<CFormView> form;
	std::shared_ptr<CDropDown> objectChoice;
	std::array<std::shared_ptr<CNumberInput>, 3> inputs;

public:
	struct Field
	{
		std::string label;
		int value;
		int minValue;
		int maxValue;
	};

	/// The object is chosen from the names unless the list of names is empty
	ObjectRowDialog(const std::string & title, const std::string & objectLabel, const std::vector<std::string> & names, int object, const std::vector<Field> & fields,
		const std::function<void(int, const std::vector<int> &)> & onSaved)
		: CDialogWindow(Point(520, 140 + static_cast<int>(fields.size()) * (UiTheme::CONTROL_HEIGHT + 6)), title)
	{
		OBJECT_CONSTRUCTION_TARGETED(content());

		const Point size = contentSize();
		form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), LABEL_WIDTH);
		objectChoice = form->addChoice(objectLabel, names, object, nullptr);
		for(size_t i = 0; i < fields.size() && i < inputs.size(); ++i)
			inputs[i] = form->addNumber(fields[i].label, fields[i].value, fields[i].minValue, fields[i].maxValue, nullptr);
		form->finish();

		const size_t fieldCount = fields.size();
		onAccept = [this, onSaved, fieldCount]()
		{
			std::vector<int> values;
			for(size_t i = 0; i < fieldCount; ++i)
				values.push_back(inputs[i]->getValue());

			onSaved(objectChoice->getSelected(), values);
			return true;
		};
	}
};
}

void EditorTemplateDialogs::editTerrains(const std::string & title, std::set<TerrainId> & target)
{
	editEntities(title, *LIBRARY->terrainTypeHandler, target);
}

void EditorTemplateDialogs::editSpells(const std::string & title, std::set<SpellID> & target)
{
	editEntities(title, *LIBRARY->spellh, target);
}

void EditorTemplateDialogs::editArtifacts(const std::string & title, std::set<ArtifactID> & target)
{
	editEntities(title, *LIBRARY->arth, target);
}

void EditorTemplateDialogs::editSkills(const std::string & title, std::set<SecondarySkill> & target)
{
	editEntities(title, *LIBRARY->skillh, target);
}

void EditorTemplateDialogs::editHeroes(const std::string & title, std::set<HeroTypeID> & target)
{
	editEntities(title, *LIBRARY->heroh, target);
}

void EditorTemplateDialogs::editFactions(const std::string & title, std::set<FactionID> & target)
{
	editEntities(title, *LIBRARY->townh, target);
}

void EditorTemplateDialogs::editMines(std::map<GameResID, ui16> & target)
{
	// resources that the generator supports for mines
	static const std::vector<GameResID> RESOURCES{GameResID::GOLD, GameResID::WOOD, GameResID::MERCURY, GameResID::ORE, GameResID::SULFUR, GameResID::CRYSTAL, GameResID::GEMS};

	class MineDialog : public CDialogWindow
	{
		std::shared_ptr<CFormView> form;
		std::vector<std::shared_ptr<CNumberInput>> inputs;

	public:
		MineDialog(std::map<GameResID, ui16> & mines)
			: CDialogWindow(Point(400, 360), EditorTexts::get("template.mines"))
		{
			OBJECT_CONSTRUCTION_TARGETED(content());

			const Point size = contentSize();
			form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), LABEL_WIDTH);
			for(const auto & resource : RESOURCES)
			{
				const auto found = mines.find(resource);
				inputs.push_back(form->addNumber(resource.toResource()->getNameTranslated(), found == mines.end() ? 0 : found->second, 0, MAX_MINES, nullptr));
			}
			form->finish();

			onAccept = [this, &mines]()
			{
				for(size_t i = 0; i < RESOURCES.size(); ++i)
					mines[RESOURCES[i]] = static_cast<ui16>(inputs[i]->getValue());
				return true;
			};
		}
	};

	ENGINE->windows().createAndPushWindow<MineDialog>(target);
}

void EditorTemplateDialogs::editTreasures(std::vector<CTreasureInfo> & target)
{
	class TreasureListDialog : public CDialogWindow
	{
		std::vector<CTreasureInfo> treasures;
		std::shared_ptr<CListEditor> list;

		void refresh(int selected)
		{
			std::vector<std::string> names;
			for(const auto & info : treasures)
				names.push_back(EditorTexts::format("template.treasureEntry", {std::to_string(info.min), std::to_string(info.max), std::to_string(info.density)}));

			list->setEntries(names, selected);
		}

		void edit(int index)
		{
			ENGINE->windows().createAndPushWindow<TreasureDialog>(treasures.at(index), [this, index](const CTreasureInfo & info)
			{
				treasures.at(index) = info;
				refresh(index);
			});
		}

	public:
		TreasureListDialog(std::vector<CTreasureInfo> & target)
			: CDialogWindow(Point(480, 380), EditorTexts::get("template.treasures"))
			, treasures(target)
		{
			OBJECT_CONSTRUCTION_TARGETED(content());

			const Point size = contentSize();
			list = std::make_shared<CListEditor>(Rect(0, 0, size.x, size.y), true);
			list->onAdd = [this]()
			{
				treasures.emplace_back(0, 0, 0);
				refresh(static_cast<int>(treasures.size()) - 1);
				edit(static_cast<int>(treasures.size()) - 1);
			};
			list->onEdit = [this](int index) { edit(index); };
			list->onRemove = [this](int index)
			{
				treasures.erase(treasures.begin() + index);
				refresh(-1);
			};
			refresh(-1);

			onAccept = [this, &target]()
			{
				target = treasures;
				return true;
			};
		}
	};

	ENGINE->windows().createAndPushWindow<TreasureListDialog>(target);
}

void EditorTemplateDialogs::editTownHints(std::vector<rmg::ZoneOptions::CTownHints> & target)
{
	class HintListDialog : public CDialogWindow
	{
		std::vector<HintDraft> hints;
		std::shared_ptr<CListEditor> list;

		void refresh(int selected)
		{
			std::vector<std::string> names;
			for(const auto & hint : hints)
				names.push_back(hintModeName(hint.mode) + ": " + hint.zones);

			list->setEntries(names, selected);
		}

		void edit(int index)
		{
			ENGINE->windows().createAndPushWindow<HintDialog>(hints.at(index), [this, index](const HintDraft & draft)
			{
				hints.at(index) = draft;
				refresh(index);
			});
		}

	public:
		HintListDialog(std::vector<rmg::ZoneOptions::CTownHints> & target)
			: CDialogWindow(Point(520, 380), EditorTexts::get("template.townHints"))
		{
			OBJECT_CONSTRUCTION_TARGETED(content());

			const Point size = contentSize();

			for(const auto & hint : target)
			{
				HintDraft draft;
				if(hint.likeZone != rmg::ZoneOptions::NO_ZONE)
				{
					draft.mode = HintMode::LIKE_ZONE;
					draft.zones = std::to_string(hint.likeZone);
				}
				else if(!hint.notLikeZone.empty())
				{
					draft.mode = HintMode::NOT_LIKE_ZONE;
					std::vector<std::string> zones;
					for(int zone : hint.notLikeZone)
						zones.push_back(std::to_string(zone));
					draft.zones = boost::algorithm::join(zones, ",");
				}
				else
				{
					draft.mode = HintMode::RELATED_TO_ZONE_TERRAIN;
					draft.zones = std::to_string(hint.relatedToZoneTerrain);
				}
				hints.push_back(draft);
			}

			list = std::make_shared<CListEditor>(Rect(0, 0, size.x, size.y), true);
			list->onAdd = [this]()
			{
				hints.emplace_back();
				refresh(static_cast<int>(hints.size()) - 1);
				edit(static_cast<int>(hints.size()) - 1);
			};
			list->onEdit = [this](int index) { edit(index); };
			list->onRemove = [this](int index)
			{
				hints.erase(hints.begin() + index);
				refresh(-1);
			};
			refresh(-1);

			onAccept = [this, &target]()
			{
				target.clear();
				for(const auto & draft : hints)
				{
					const std::vector<int> zones = parseZones(draft.zones);
					if(zones.empty())
						continue;

					rmg::ZoneOptions::CTownHints hint;
					switch(draft.mode)
					{
						case HintMode::LIKE_ZONE:
							hint.likeZone = zones.front();
							break;
						case HintMode::NOT_LIKE_ZONE:
							hint.notLikeZone = zones;
							break;
						case HintMode::RELATED_TO_ZONE_TERRAIN:
							hint.relatedToZoneTerrain = zones.front();
							break;
					}
					target.push_back(hint);
				}
				return true;
			};
		}
	};

	ENGINE->windows().createAndPushWindow<HintListDialog>(target);
}

// ---------------------------------------------------------------------------------------------

EditorTemplateObjectDialog::EditorTemplateObjectDialog(ObjectConfig & config, IGameInfoCallback * callback, const std::function<void()> & onChanged)
	: CDialogWindow(Point(720, 520), EditorTexts::get("template.objects"))
	, config(config)
	, bannedCategories(config.bannedObjectCategories)
	, bannedObjects(config.bannedObjects)
	, customObjects(config.customObjects)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();

	for(const auto & [id, info] : config.getRequiredObjects())
		requiredObjects.push_back({id, info.first, info.second ? static_cast<int>(*info.second) : -1});

	std::map<CompoundMapObjectID, std::string> names;
	for(const auto & id : LIBRARY->objtypeh->knownObjects())
	{
		for(const auto & subId : LIBRARY->objtypeh->knownSubObjects(id))
		{
			const auto factory = LIBRARY->objtypeh->getHandlerFor(id, subId);

			std::string name;
			try
			{
				const auto templates = factory->getTemplates();
				if(!templates.empty())
					name = EditorTexts::translate(factory->create(callback, templates.front())->getObjectName());
			}
			catch(const std::exception & e)
			{
				logGlobal->debug("Object %d:%d can not be created to get its name: %s", id.getNum(), subId.getNum(), e.what());
			}

			// no object could be created, so the name of the type is used
			if(name.empty())
				name = EditorTexts::translate(TextIdentifier(factory->hasNameTextID() ? factory->getNameTextID() : LIBRARY->objtypeh->getObjectGroupNameTextID(id)));

			if(!name.empty())
				names[CompoundMapObjectID(id.getNum(), subId.getNum())] = name;
		}
	}

	for(const auto & [id, name] : names)
	{
		objectIds.push_back(id);
		objectNames.push_back(name);
	}

	tabs = std::make_shared<CTabBar>(Rect(0, 0, size.x, UiTheme::CONTROL_HEIGHT + 4),
		std::vector<std::string>{EditorTexts::get("template.bannedCategories"), EditorTexts::get("template.bannedObjects"), EditorTexts::get("template.customObjects"), EditorTexts::get("template.requiredObjects")},
		0, [this](int index) { selectTab(index); });

	const Rect listArea(0, UiTheme::CONTROL_HEIGHT + 4 + UiTheme::SPACING, size.x, size.y - UiTheme::CONTROL_HEIGHT - 4 - UiTheme::SPACING);
	for(int tab = 0; tab < 4; ++tab)
	{
		lists[tab] = std::make_shared<CListEditor>(listArea, true);
		lists[tab]->onEdit = [this, tab](int index)
		{
			switch(tab)
			{
				case 0: editCategory(index); break;
				case 1: editBanned(index); break;
				case 2: editCustom(index); break;
				default: editRequired(index); break;
			}
		};
	}

	lists[0]->onAdd = [this]() { bannedCategories.push_back(ObjectConfig::EObjectCategory::ALL); refreshList(0, static_cast<int>(bannedCategories.size()) - 1); editCategory(static_cast<int>(bannedCategories.size()) - 1); };
	lists[1]->onAdd = [this]() { bannedObjects.push_back(objectIds.front()); refreshList(1, static_cast<int>(bannedObjects.size()) - 1); editBanned(static_cast<int>(bannedObjects.size()) - 1); };
	lists[2]->onAdd = [this]()
	{
		ObjectInfo info(objectIds.front());
		info.value = 0;
		info.probability = 0;
		info.maxPerZone = 1;
		customObjects.push_back(info);
		refreshList(2, static_cast<int>(customObjects.size()) - 1);
		editCustom(static_cast<int>(customObjects.size()) - 1);
	};
	lists[3]->onAdd = [this]() { requiredObjects.push_back({objectIds.front(), 1, -1}); refreshList(3, static_cast<int>(requiredObjects.size()) - 1); editRequired(static_cast<int>(requiredObjects.size()) - 1); };

	lists[0]->onRemove = [this](int index) { bannedCategories.erase(bannedCategories.begin() + index); refreshList(0, -1); };
	lists[1]->onRemove = [this](int index) { bannedObjects.erase(bannedObjects.begin() + index); refreshList(1, -1); };
	lists[2]->onRemove = [this](int index) { customObjects.erase(customObjects.begin() + index); refreshList(2, -1); };
	lists[3]->onRemove = [this](int index) { requiredObjects.erase(requiredObjects.begin() + index); refreshList(3, -1); };

	for(int tab = 0; tab < 4; ++tab)
		refreshList(tab, -1);

	selectTab(0);

	onAccept = [this, onChanged]()
	{
		commit();
		if(onChanged)
			onChanged();
		return true;
	};
}

std::string EditorTemplateObjectDialog::objectName(const CompoundMapObjectID & id) const
{
	const auto found = std::ranges::find(objectIds, id);
	return found == objectIds.end() ? std::to_string(id.primaryID) + ":" + std::to_string(id.secondaryID) : objectNames[found - objectIds.begin()];
}

void EditorTemplateObjectDialog::selectTab(int index)
{
	for(int i = 0; i < 4; ++i)
		lists[i]->setEnabled(i == index);

	tabs->setSelected(index);
	redraw();
}

void EditorTemplateObjectDialog::refreshList(int tab, int selected)
{
	std::vector<std::string> names;

	switch(tab)
	{
		case 0:
			for(const auto & category : bannedCategories)
				names.push_back(categoryName(category));
			break;
		case 1:
			for(const auto & id : bannedObjects)
				names.push_back(objectName(id));
			break;
		case 2:
			for(const auto & info : customObjects)
				names.push_back(EditorTexts::format("template.customEntry", {objectName(info.getCompoundID()), std::to_string(info.value), std::to_string(info.probability), std::to_string(info.maxPerZone)}));
			break;
		default:
			for(const auto & required : requiredObjects)
				names.push_back(EditorTexts::format("template.requiredEntry", {objectName(required.object), std::to_string(required.count), required.guard < 0 ? EditorTexts::get("template.noGuard") : std::to_string(required.guard)}));
			break;
	}

	lists[tab]->setEntries(names, selected);
}

void EditorTemplateObjectDialog::editCategory(int index)
{
	const auto & categories = allCategories();
	std::vector<std::string> names;
	for(const auto & category : categories)
		names.push_back(categoryName(category));

	const int current = static_cast<int>(std::ranges::find(categories, bannedCategories.at(index)) - categories.begin());
	CChoiceDialog::chooseOne(EditorTexts::get("template.bannedCategories"), CChoiceDialog::makeItems(names), current, [this, index](int chosen)
	{
		bannedCategories.at(index) = allCategories().at(chosen);
		refreshList(0, index);
	});
}

void EditorTemplateObjectDialog::editBanned(int index)
{
	const int current = static_cast<int>(std::ranges::find(objectIds, bannedObjects.at(index)) - objectIds.begin());
	CChoiceDialog::chooseOne(EditorTexts::get("template.bannedObjects"), CChoiceDialog::makeItems(objectNames), current < static_cast<int>(objectIds.size()) ? current : 0, [this, index](int chosen)
	{
		bannedObjects.at(index) = objectIds.at(chosen);
		refreshList(1, index);
	});
}

void EditorTemplateObjectDialog::editCustom(int index)
{
	const ObjectInfo & info = customObjects.at(index);
	const int current = static_cast<int>(std::ranges::find(objectIds, info.getCompoundID()) - objectIds.begin());
	const std::vector<ObjectRowDialog::Field> fields{
		{EditorTexts::get("template.value"), static_cast<int>(info.value), 0, MAX_OBJECT_VALUE},
		{EditorTexts::get("template.probability"), static_cast<int>(info.probability), 0, MAX_OBJECT_PROBABILITY},
		{EditorTexts::get("template.maxPerZone"), static_cast<int>(info.maxPerZone), 0, MAX_OBJECTS_PER_ZONE}};

	ENGINE->windows().createAndPushWindow<ObjectRowDialog>(EditorTexts::get("template.customObjects"), EditorTexts::get("template.object"), objectNames,
		current < static_cast<int>(objectIds.size()) ? current : 0, fields, [this, index](int object, const std::vector<int> & values)
		{
			ObjectInfo edited(objectIds.at(object));
			edited.value = values.at(0);
			edited.probability = values.at(1);
			edited.maxPerZone = values.at(2);
			customObjects.at(index) = edited;
			refreshList(2, index);
		});
}

void EditorTemplateObjectDialog::editRequired(int index)
{
	const Required & required = requiredObjects.at(index);
	const int current = static_cast<int>(std::ranges::find(objectIds, required.object) - objectIds.begin());
	// -1 means that the object is not guarded
	const std::vector<ObjectRowDialog::Field> fields{
		{EditorTexts::get("template.count"), required.count, 1, MAX_REQUIRED_COUNT},
		{EditorTexts::get("template.guard"), required.guard, -1, MAX_GUARD}};

	ENGINE->windows().createAndPushWindow<ObjectRowDialog>(EditorTexts::get("template.requiredObjects"), EditorTexts::get("template.object"), objectNames,
		current < static_cast<int>(objectIds.size()) ? current : 0, fields, [this, index](int object, const std::vector<int> & values)
		{
			requiredObjects.at(index) = {objectIds.at(object), values.at(0), values.at(1)};
			refreshList(3, index);
		});
}

void EditorTemplateObjectDialog::commit()
{
	config.bannedObjectCategories = bannedCategories;
	config.bannedObjects = bannedObjects;
	config.customObjects = customObjects;

	config.clearRequiredObjects();
	for(const auto & required : requiredObjects)
		config.addRequiredObject(required.object, static_cast<ui16>(required.count), required.guard >= 0 ? std::optional<ui32>(required.guard) : std::nullopt);
}
