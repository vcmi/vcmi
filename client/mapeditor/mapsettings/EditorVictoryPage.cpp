/*
 * EditorVictoryPage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorVictoryPage.h"

#include "../EditorMap.h"
#include "../EditorMapHelpers.h"
#include "../EditorTexts.h"

#include "../../widgets/CCheckBox.h"
#include "../../widgets/CFormView.h"

#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/constants/StringConstants.h"
#include "../../../lib/entities/ResourceTypeHandler.h"
#include "../../../lib/entities/artifact/CArtHandler.h"
#include "../../../lib/entities/faction/CTown.h"
#include "../../../lib/entities/faction/CTownHandler.h"
#include "../../../lib/mapObjects/CGCreature.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/mapping/CMapEditManager.h"
#include "../../../lib/texts/MetaString.h"


namespace
{
constexpr int LABEL_WIDTH = 190;

int indexOf(const std::vector<int> & values, int value)
{
	auto it = std::find(values.begin(), values.end(), value);
	return it == values.end() ? 0 : static_cast<int>(it - values.begin());
}
}

EditorVictoryPage::EditorVictoryPage(EditorMap & editor, const Rect & area)
	: EditorSettingsPage(editor, area)
{
	OBJECT_CONSTRUCTION;

	form = std::make_shared<CFormView>(Rect(0, 0, area.w, area.h), LABEL_WIDTH);

	loadFromMap();
	rebuildForm();
}

void EditorVictoryPage::prepareChoices()
{
	primaryValues.clear();
	primaryNames.clear();
	secondaryValues.clear();
	secondaryNames.clear();

	const CMap & m = map();

	auto addArtifacts = [this]()
	{
		for(size_t i = 0; i < LIBRARY->arth->objects.size(); ++i)
		{
			primaryValues.push_back(static_cast<int>(i));
			primaryNames.push_back(LIBRARY->arth->objects[i]->getNameTranslated());
		}
	};
	auto addTowns = [&m](std::vector<int> & values, std::vector<std::string> & names)
	{
		for(int i : EditorMapHelpers::getObjectIndexes<const CGTownInstance>(m))
		{
			values.push_back(i);
			names.push_back(EditorMapHelpers::getTownName(m, i));
		}
	};

	switch(condition)
	{
		case ARTIFACT:
			addArtifacts();
			break;

		case CREATURES:
			for(size_t i = 0; i < LIBRARY->creh->objects.size(); ++i)
			{
				primaryValues.push_back(static_cast<int>(i));
				primaryNames.push_back(LIBRARY->creh->objects[i]->getNamePluralTranslated());
			}
			break;

		case RESOURCES:
			for(auto & resource : LIBRARY->resourceTypeHandler->getAllObjects())
			{
				MetaString name;
				name.appendName(GameResID(resource));
				primaryValues.push_back(resource.getNum());
				primaryNames.push_back(EditorTexts::translate(name));
			}
			break;

		case BUILDING:
		{
			auto * ctown = LIBRARY->townh->randomFaction->town.get();
			for(int bId : ctown->getAllBuildings())
			{
				primaryValues.push_back(bId);
				primaryNames.push_back(EditorMapHelpers::defaultBuildingName(BuildingID(bId)));
			}

			secondaryValues.push_back(-1);
			secondaryNames.push_back(EditorTexts::get("victory.anyTown"));
			addTowns(secondaryValues, secondaryNames);
			break;
		}

		case TOWN:
			addTowns(primaryValues, primaryNames);
			break;

		case HERO:
			for(int i : EditorMapHelpers::getHeroTargetObjectIndexes(m))
			{
				primaryValues.push_back(i);
				primaryNames.push_back(EditorMapHelpers::getHeroName(m, i));
			}
			break;

		case TRANSPORT:
			addArtifacts();
			addTowns(secondaryValues, secondaryNames);
			break;

		case MONSTER:
			for(int i : EditorMapHelpers::getObjectIndexes<const CGCreature>(m))
			{
				primaryValues.push_back(i);
				primaryNames.push_back(EditorMapHelpers::getMonsterName(m, i));
			}
			break;

		default:
			break;
	}
}

void EditorVictoryPage::loadFromMap()
{
	const CMap & m = map();

	standardVictory = false;
	onlyForHumans = false;
	condition = NONE;
	prepareChoices();

	for(auto & ev : m.triggeredEvents)
	{
		if(ev.effect.type != EventEffect::VICTORY)
			continue;

		if(ev.identifier == "standardVictory")
			standardVictory = true;

		if(ev.identifier != "specialVictory")
			continue;

		auto readJson = ev.trigger.toJson(EditorMapHelpers::conditionToJson);
		for(auto & json : EditorMapHelpers::linearJsonArray(readJson))
		{
			switch(json["condition"].Integer())
			{
				case EventCondition::HAVE_ARTIFACT:
					condition = ARTIFACT;
					prepareChoices();
					primary = indexOf(primaryValues, static_cast<int>(ArtifactID::decode(json["objectType"].String())));
					break;

				case EventCondition::HAVE_CREATURES:
					condition = CREATURES;
					prepareChoices();
					primary = indexOf(primaryValues, static_cast<int>(CreatureID::decode(json["objectType"].String())));
					amount = static_cast<int>(json["value"].Integer());
					break;

				case EventCondition::HAVE_RESOURCES:
					condition = RESOURCES;
					prepareChoices();
					primary = indexOf(primaryValues, static_cast<int>(EGameResID::decode(json["objectType"].String())));
					amount = static_cast<int>(json["value"].Integer());
					break;

				case EventCondition::HAVE_BUILDING:
				{
					condition = BUILDING;
					prepareChoices();
					primary = indexOf(primaryValues, static_cast<int>(BuildingID::decode(json["objectType"].String())));
					const int townIdx = EditorMapHelpers::getObjectByPos<const CGTownInstance>(m, EditorMapHelpers::posFromJson(json["position"]));
					if(townIdx >= 0)
						secondary = indexOf(secondaryValues, townIdx);
					break;
				}

				case EventCondition::CONTROL:
				case EventCondition::CONTROL_CURRENT:
				{
					auto mapObject = MapObjectID::decode(json["objectType"].String());
					if(mapObject == Obj::MINE)
					{
						condition = MINES;
						break;
					}

					condition = TOWN;
					prepareChoices();
					if(mapObject == Obj::TOWN)
					{
						const int townIdx = EditorMapHelpers::getObjectByPos<const CGTownInstance>(m, EditorMapHelpers::posFromJson(json["position"]));
						if(townIdx >= 0)
							primary = indexOf(primaryValues, townIdx);
					}
					//TODO: support control other objects (dwellings)
					break;
				}

				case EventCondition::DESTROY:
				{
					auto objectType = MapObjectID::decode(json["objectType"].String());
					if(objectType == Obj::HERO || objectType == Obj::HERO_PLACEHOLDER)
					{
						condition = HERO;
						prepareChoices();
						const int heroIdx = EditorMapHelpers::getHeroTargetObjectByPos(m, EditorMapHelpers::posFromJson(json["position"]));
						if(heroIdx >= 0)
							primary = indexOf(primaryValues, heroIdx);
					}
					if(objectType == Obj::MONSTER)
					{
						condition = MONSTER;
						prepareChoices();
						const int monsterIdx = EditorMapHelpers::getObjectByPos<const CGCreature>(m, EditorMapHelpers::posFromJson(json["position"]));
						if(monsterIdx >= 0)
							primary = indexOf(primaryValues, monsterIdx);
					}
					break;
				}

				case EventCondition::TRANSPORT:
				{
					condition = TRANSPORT;
					prepareChoices();
					primary = indexOf(primaryValues, static_cast<int>(ArtifactID::decode(json["objectType"].String())));
					const int townIdx = EditorMapHelpers::getObjectByPos<const CGTownInstance>(m, EditorMapHelpers::posFromJson(json["position"]));
					if(townIdx >= 0)
						secondary = indexOf(secondaryValues, townIdx);
					break;
				}

				case EventCondition::IS_HUMAN:
					onlyForHumans = true;
					break;
			}
		}
	}

	if(condition == NONE)
		standardVictory = true;
}

void EditorVictoryPage::rebuildForm()
{
	form->clear();

	std::vector<std::string> conditionNames;
	for(const char * key : {"none", "artifact", "creatures", "resources", "building", "town", "hero", "transport", "monster", "mines"})
		conditionNames.push_back(EditorTexts::get(std::string("victory.") + key));

	form->addChoice(EditorTexts::get("victory.condition"), conditionNames, condition, [this](int index)
	{
		condition = index;
		primary = 0;
		secondary = 0;
		amount = 1;

		if(condition == NONE)
		{
			standardVictory = true;
			onlyForHumans = false;
		}

		prepareChoices();
		rebuildForm();
	});

	static const std::map<int, std::pair<const char *, const char *>> labels = {
		{ARTIFACT, {"victory.artifact", nullptr}},
		{CREATURES, {"victory.creature", nullptr}},
		{RESOURCES, {"victory.resource", nullptr}},
		{BUILDING, {"victory.building", "victory.town"}},
		{TOWN, {"victory.town", nullptr}},
		{HERO, {"victory.hero", nullptr}},
		{TRANSPORT, {"victory.artifact", "victory.destination"}},
		{MONSTER, {"victory.monster", nullptr}},
	};

	auto labelIt = labels.find(condition);
	if(labelIt != labels.end())
	{
		form->addChoice(EditorTexts::get(labelIt->second.first), primaryNames, primary, [this](int index) { primary = index; });

		if(condition == CREATURES || condition == RESOURCES)
			form->addNumber(EditorTexts::get("victory.amount"), amount, 1, 999999999, [this](int value) { amount = value; });

		if(labelIt->second.second)
			form->addChoice(EditorTexts::get(labelIt->second.second), secondaryNames, secondary, [this](int index) { secondary = index; });

		if(condition == BUILDING || condition == TOWN || condition == HERO || condition == TRANSPORT || condition == MONSTER)
			form->addButton("", EditorTexts::get("victory.pick"), [this]() { pickObject(); });
	}

	auto standardCheck = form->addCheck(EditorTexts::get("victory.standard"), standardVictory, [this](bool on) { standardVictory = on; });
	auto humansCheck = form->addCheck(EditorTexts::get("victory.onlyHumans"), onlyForHumans, [this](bool on) { onlyForHumans = on; });
	standardCheck->block(condition == NONE);
	humansCheck->block(condition == NONE);

	form->addInfo(EditorTexts::get("victory.message"), EditorTexts::translate(map().victoryMessage));
	form->finish();
}

void EditorVictoryPage::pickObject()
{
	EditorMapHelpers::PickFilter filter;
	switch(condition)
	{
		case BUILDING:
		case TOWN:
		case TRANSPORT:
			filter = [](const CGObjectInstance * object) { return dynamic_cast<const CGTownInstance *>(object) != nullptr; };
			break;
		case HERO:
			filter = [](const CGObjectInstance * object) { return EditorMapHelpers::isHeroTargetObject(object); };
			break;
		case MONSTER:
			filter = [](const CGObjectInstance * object) { return dynamic_cast<const CGCreature *>(object) != nullptr; };
			break;
		default:
			return;
	}

	const bool usesSecondary = condition == BUILDING || condition == TRANSPORT;
	EditorMapHelpers::pickObject(editor, 1, filter, [this, usesSecondary](const CGObjectInstance * object)
	{
		if(!object)
			return;

		const auto & values = usesSecondary ? secondaryValues : primaryValues;
		for(size_t i = 0; i < values.size(); ++i)
		{
			if(values[i] >= 0 && map().objects.at(values[i]).get() == object)
			{
				(usesSecondary ? secondary : primary) = static_cast<int>(i);
				rebuildForm();
				break;
			}
		}
	});
}

void EditorVictoryPage::apply()
{
	CMap & m = map();

	// a condition that needs an object is useless without one
	if((condition == TOWN || condition == HERO || condition == MONSTER) && primaryValues.empty())
	{
		condition = NONE;
		standardVictory = true;
		onlyForHumans = false;
	}

	//victory conditions
	EventCondition victoryCondition(EventCondition::STANDARD_WIN);

	//Victory condition - defeat all
	TriggeredEvent standardEvent;
	standardEvent.effect.type = EventEffect::VICTORY;
	standardEvent.effect.toOtherMessage.appendTextID("core.genrltxt.5");
	standardEvent.identifier = "standardVictory";
	standardEvent.description.clear(); // TODO: display in quest window
	standardEvent.onFulfill.appendTextID("core.genrltxt.659");
	standardEvent.trigger = EventExpression(victoryCondition);

	if(condition == NONE)
	{
		m.triggeredEvents.push_back(standardEvent);
		m.victoryIconIndex = 11;
		m.victoryMessage = MetaString::createFromTextID("core.vcdesc.0");
		return;
	}

	const int vicCondition = condition - 1;

	TriggeredEvent specialVictory;
	specialVictory.effect.type = EventEffect::VICTORY;
	specialVictory.identifier = "specialVictory";
	specialVictory.description.clear(); // TODO: display in quest window

	m.victoryIconIndex = vicCondition;
	m.victoryMessage = MetaString::createFromTextID("core.vcdesc", vicCondition + 1);

	const int primaryValue = primary < static_cast<int>(primaryValues.size()) ? primaryValues[primary] : 0;
	const int secondaryValue = secondary < static_cast<int>(secondaryValues.size()) ? secondaryValues[secondary] : -1;

	switch(condition)
	{
		case ARTIFACT:
		{
			EventCondition cond(EventCondition::HAVE_ARTIFACT);
			cond.objectType = ArtifactID(primaryValue);
			specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.281");
			specialVictory.onFulfill.appendTextID("core.genrltxt.280");
			specialVictory.trigger = EventExpression(cond);
			break;
		}

		case CREATURES:
		{
			EventCondition cond(EventCondition::HAVE_CREATURES);
			cond.objectType = CreatureID(primaryValue);
			cond.value = amount;
			specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.277");
			specialVictory.onFulfill.appendTextID("core.genrltxt.276");
			specialVictory.trigger = EventExpression(cond);
			break;
		}

		case RESOURCES:
		{
			EventCondition cond(EventCondition::HAVE_RESOURCES);
			cond.objectType = GameResID(primaryValue);
			cond.value = amount;
			specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.279");
			specialVictory.onFulfill.appendTextID("core.genrltxt.278");
			specialVictory.trigger = EventExpression(cond);
			break;
		}

		case BUILDING:
		{
			EventCondition cond(EventCondition::HAVE_BUILDING);
			cond.objectType = BuildingID(primaryValue);
			if(secondaryValue > -1)
				cond.position = m.objects[secondaryValue]->pos;
			specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.283");
			specialVictory.onFulfill.appendTextID("core.genrltxt.282");
			specialVictory.trigger = EventExpression(cond);
			break;
		}

		case TOWN:
		{
			EventCondition cond(EventCondition::CONTROL_CURRENT);
			cond.objectType = Obj(Obj::TOWN);
			cond.position = m.objects[primaryValue]->pos;
			specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.250");
			specialVictory.onFulfill.appendTextID("core.genrltxt.249");
			specialVictory.trigger = EventExpression(cond);
			break;
		}

		case HERO:
		{
			EventCondition cond(EventCondition::DESTROY);
			cond.objectType = Obj(Obj::HERO);
			cond.position = m.objects[primaryValue]->pos;
			specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.253");
			specialVictory.onFulfill.appendTextID("core.genrltxt.252");
			specialVictory.trigger = EventExpression(cond);
			break;
		}

		case TRANSPORT:
		{
			EventCondition cond(EventCondition::TRANSPORT);
			cond.objectType = ArtifactID(primaryValue);
			if(secondaryValue > -1)
				cond.position = m.objects[secondaryValue]->pos;
			specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.293");
			specialVictory.onFulfill.appendTextID("core.genrltxt.292");
			specialVictory.trigger = EventExpression(cond);
			break;
		}

		case MONSTER:
		{
			EventCondition cond(EventCondition::DESTROY);
			cond.objectType = Obj(Obj::MONSTER);
			cond.position = m.objects[primaryValue]->pos;
			specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.287");
			specialVictory.onFulfill.appendTextID("core.genrltxt.286");
			specialVictory.trigger = EventExpression(cond);
			break;
		}

		case MINES:
		{
			EventCondition cond(EventCondition::CONTROL_CURRENT);
			cond.objectType = Obj(Obj::MINE);
			specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.291");
			specialVictory.onFulfill.appendTextID("core.genrltxt.290");
			specialVictory.trigger = EventExpression(cond);
			m.victoryIconIndex = 9;
			m.victoryMessage = MetaString::createFromTextID("core.vcdesc.10");
			break;
		}

		default:
			break;
	}

	// if condition is human-only turn it into following construction: AllOf(human, condition)
	if(onlyForHumans)
	{
		EventExpression::OperatorAll oper;
		EventCondition notAI(EventCondition::IS_HUMAN);
		notAI.value = 1;
		oper.expressions.push_back(notAI);
		oper.expressions.push_back(specialVictory.trigger.get());
		specialVictory.trigger = EventExpression(oper);
	}

	// if normal victory allowed - add one more quest
	if(standardVictory)
	{
		m.victoryMessage.appendRawString(" / ");
		m.victoryMessage.appendTextID("core.vcdesc.0");
		m.triggeredEvents.push_back(standardEvent);
	}
	m.triggeredEvents.push_back(specialVictory);
}
