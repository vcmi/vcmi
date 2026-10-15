/*
 * EditorLossPage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorLossPage.h"

#include "../EditorMap.h"
#include "../EditorMapHelpers.h"
#include "../EditorTexts.h"

#include "../../widgets/CCheckBox.h"
#include "../../widgets/CFormView.h"
#include "../../widgets/CTextField.h"

#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/mapping/CMapEditManager.h"
#include "../../../lib/texts/MetaString.h"

namespace
{
constexpr int LABEL_WIDTH = 190;
constexpr int DAYS_IN_WEEK = 7;
constexpr int DAYS_IN_MONTH = 28;

int indexOf(const std::vector<int> & values, int value)
{
	auto it = std::find(values.begin(), values.end(), value);
	return it == values.end() ? 0 : static_cast<int>(it - values.begin());
}
}

EditorLossPage::EditorLossPage(EditorMap & editor, const Rect & area)
	: EditorSettingsPage(editor, area)
{
	OBJECT_CONSTRUCTION;

	form = std::make_shared<CFormView>(Rect(0, 0, area.w, area.h), LABEL_WIDTH);

	message = EditorTexts::translate(map().defeatMessage);
	loadFromMap();
	rebuildForm();
}

void EditorLossPage::prepareChoices()
{
	objectValues.clear();
	objectNames.clear();

	const CMap & m = map();

	if(condition == TOWN)
	{
		for(int i : EditorMapHelpers::getObjectIndexes<const CGTownInstance>(m))
		{
			objectValues.push_back(i);
			objectNames.push_back(EditorMapHelpers::getTownName(m, i));
		}
	}

	if(condition == HERO)
	{
		for(int i : EditorMapHelpers::getHeroTargetObjectIndexes(m))
		{
			objectValues.push_back(i);
			objectNames.push_back(EditorMapHelpers::getHeroName(m, i));
		}
	}
}

void EditorLossPage::loadFromMap()
{
	const CMap & m = map();

	standardDefeat = false;
	condition = NONE;
	prepareChoices();

	for(auto & ev : m.triggeredEvents)
	{
		if(ev.effect.type != EventEffect::DEFEAT)
			continue;

		if(ev.identifier == "standardDefeat")
			standardDefeat = true;

		if(ev.identifier != "specialDefeat")
			continue;

		auto readJson = ev.trigger.toJson(EditorMapHelpers::conditionToJson);
		for(auto & json : EditorMapHelpers::linearJsonArray(readJson))
		{
			switch(json["condition"].Integer())
			{
				case EventCondition::CONTROL:
				{
					auto objectType = MapObjectID::decode(json["objectType"].String());
					if(objectType == Obj::TOWN)
					{
						condition = TOWN;
						prepareChoices();
						const int townIdx = EditorMapHelpers::getObjectByPos<const CGTownInstance>(m, EditorMapHelpers::posFromJson(json["position"]));
						if(townIdx >= 0)
							selectedObject = indexOf(objectValues, townIdx);
					}
					if(objectType == Obj::HERO || objectType == Obj::HERO_PLACEHOLDER)
					{
						condition = HERO;
						prepareChoices();
						const int heroIdx = EditorMapHelpers::getHeroTargetObjectByPos(m, EditorMapHelpers::posFromJson(json["position"]));
						if(heroIdx >= 0)
							selectedObject = indexOf(objectValues, heroIdx);
					}
					break;
				}

				case EventCondition::DAYS_PASSED:
				{
					condition = TIME;
					const int value = static_cast<int>(json["value"].Integer());
					days = value % DAYS_IN_WEEK;
					weeks = (value % DAYS_IN_MONTH) / DAYS_IN_WEEK;
					months = value / DAYS_IN_MONTH;
					break;
				}

				case EventCondition::DAYS_WITHOUT_TOWN:
					condition = DAYS_WITHOUT_TOWN;
					daysWithoutTown = static_cast<int>(json["value"].Integer());
					break;

				case EventCondition::IS_HUMAN:
					break; //ignore because always applicable for defeat conditions
			}
		}
	}

	if(condition == NONE)
		standardDefeat = true;
}

void EditorLossPage::rebuildForm()
{
	form->clear();

	std::vector<std::string> conditionNames;
	for(const char * key : {"none", "town", "hero", "time", "daysWithoutTown"})
		conditionNames.push_back(EditorTexts::get(std::string("loss.") + key));

	form->addChoice(EditorTexts::get("loss.condition"), conditionNames, condition, [this](int index)
	{
		condition = index;
		selectedObject = 0;
		if(condition == NONE)
			standardDefeat = true;

		prepareChoices();
		rebuildForm();
	});

	if(condition == TOWN || condition == HERO)
	{
		form->addChoice(EditorTexts::get(condition == TOWN ? "loss.townLabel" : "loss.heroLabel"), objectNames, selectedObject, [this](int index) { selectedObject = index; });
		form->addButton("", EditorTexts::get("loss.pick"), [this]() { pickObject(); });
	}

	if(condition == TIME)
	{
		form->addNumber(EditorTexts::get("loss.day"), days, 0, DAYS_IN_WEEK - 1, [this](int value) { days = value; });
		form->addNumber(EditorTexts::get("loss.week"), weeks, 0, DAYS_IN_MONTH / DAYS_IN_WEEK - 1, [this](int value) { weeks = value; });
		form->addNumber(EditorTexts::get("loss.month"), months, 0, 9999, [this](int value) { months = value; });
	}

	if(condition == DAYS_WITHOUT_TOWN)
		form->addNumber(EditorTexts::get("loss.daysCount"), daysWithoutTown, 1, 9999, [this](int value) { daysWithoutTown = value; });

	auto standardCheck = form->addCheck(EditorTexts::get("loss.standard"), standardDefeat, [this](bool on) { standardDefeat = on; });
	standardCheck->block(condition == NONE);

	form->addText(EditorTexts::get("loss.message"), message, [this](const std::string & text) { message = text; });
	form->finish();
}

void EditorLossPage::pickObject()
{
	EditorMapHelpers::PickFilter filter;
	if(condition == TOWN)
		filter = [](const CGObjectInstance * object) { return dynamic_cast<const CGTownInstance *>(object) != nullptr; };
	else if(condition == HERO)
		filter = [](const CGObjectInstance * object) { return EditorMapHelpers::isHeroTargetObject(object); };
	else
		return;

	EditorMapHelpers::pickObject(editor, 1, filter, [this](const CGObjectInstance * object)
	{
		if(!object)
			return;

		for(size_t i = 0; i < objectValues.size(); ++i)
		{
			if(map().objects.at(objectValues[i]).get() == object)
			{
				selectedObject = static_cast<int>(i);
				rebuildForm();
				break;
			}
		}
	});
}

void EditorLossPage::apply()
{
	CMap & m = map();

	// a condition that needs an object is useless without one
	if((condition == TOWN || condition == HERO) && objectValues.empty())
	{
		condition = NONE;
		standardDefeat = true;
	}

	bool customMessage = true;

	//loss conditions
	EventCondition defeatCondition(EventCondition::DAYS_WITHOUT_TOWN);
	defeatCondition.value = 7;

	//Loss condition - 7 days without town
	TriggeredEvent standardEvent;
	standardEvent.effect.type = EventEffect::DEFEAT;
	standardEvent.effect.toOtherMessage.appendTextID("core.genrltxt.8");
	standardEvent.identifier = "standardDefeat";
	standardEvent.description.clear(); // TODO: display in quest window
	standardEvent.onFulfill.appendTextID("core.genrltxt.7");
	standardEvent.trigger = EventExpression(defeatCondition);

	if(condition == NONE)
	{
		m.triggeredEvents.push_back(standardEvent);
		m.defeatIconIndex = 3;
		m.defeatMessage = MetaString::createFromTextID("core.lcdesc.0");
		customMessage = false;
	}
	else
	{
		const int lossCondition = condition - 1;
		const int objectValue = selectedObject < static_cast<int>(objectValues.size()) ? objectValues[selectedObject] : 0;

		TriggeredEvent specialDefeat;
		specialDefeat.effect.type = EventEffect::DEFEAT;
		specialDefeat.identifier = "specialDefeat";
		specialDefeat.description.clear(); // TODO: display in quest window

		m.defeatIconIndex = lossCondition;

		switch(condition)
		{
			case TOWN:
			{
				EventExpression::OperatorNone noneOf;
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj(Obj::TOWN);
				cond.position = m.objects[objectValue]->pos;
				noneOf.expressions.push_back(cond);
				specialDefeat.onFulfill.appendTextID("core.genrltxt.251");
				specialDefeat.trigger = EventExpression(noneOf);
				m.defeatMessage = MetaString::createFromTextID("core.lcdesc.1");
				customMessage = false;
				break;
			}

			case HERO:
			{
				EventExpression::OperatorNone noneOf;
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj(Obj::HERO);
				cond.position = m.objects[objectValue]->pos;
				noneOf.expressions.push_back(cond);
				specialDefeat.onFulfill.appendTextID("core.genrltxt.253");
				specialDefeat.trigger = EventExpression(noneOf);
				m.defeatMessage = MetaString::createFromTextID("core.lcdesc.2");
				customMessage = false;
				break;
			}

			case TIME:
			{
				EventCondition cond(EventCondition::DAYS_PASSED);
				cond.value = months * DAYS_IN_MONTH + weeks * DAYS_IN_WEEK + days;
				specialDefeat.onFulfill.appendTextID("core.genrltxt.254");
				specialDefeat.trigger = EventExpression(cond);
				m.defeatMessage = MetaString::createFromTextID("core.lcdesc.3");
				customMessage = false;
				break;
			}

			case DAYS_WITHOUT_TOWN:
			{
				EventCondition cond(EventCondition::DAYS_WITHOUT_TOWN);
				cond.value = daysWithoutTown;
				specialDefeat.onFulfill.appendTextID("core.genrltxt.7");
				specialDefeat.trigger = EventExpression(cond);
				break;
			}

			default:
				break;
		}

		EventExpression::OperatorAll allOf;
		EventCondition isHuman(EventCondition::IS_HUMAN);
		isHuman.value = 1;
		allOf.expressions.push_back(specialDefeat.trigger.get());
		allOf.expressions.push_back(isHuman);
		specialDefeat.trigger = EventExpression(allOf);

		if(standardDefeat)
			m.triggeredEvents.push_back(standardEvent);

		m.triggeredEvents.push_back(specialDefeat);
	}

	if(customMessage)
		m.defeatMessage = MetaString::createFromTextID(mapRegisterLocalizedString("map", m, TextIdentifier("header", "defeatMessage"), message));
}
