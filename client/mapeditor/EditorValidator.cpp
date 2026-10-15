/*
 * EditorValidator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorValidator.h"

#include "EditorMap.h"
#include "EditorTexts.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CFormView.h"
#include "../widgets/UiTheme.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/json/JsonKeyExtractor.h"
#include "../../lib/mapObjects/MapObjects.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/modding/ModDescription.h"

#include <vcmi/spells/Spell.h>

namespace
{
enum class PresetState
{
	VALID,
	OVERWRITES_MAP_SETTINGS,
	ILLEGAL,
	INVALID
};

template<typename IdentifierType>
PresetState validatePreset(const CMap * map, const CGObjectInstance * object, const std::string & category, const std::string & name, const std::set<IdentifierType> & allowed)
{
	JsonKeyExtractor keyExtractor(map->cb);
	const auto * rewardable = dynamic_cast<const CRewardableObject *>(object);
	if(!rewardable)
		return PresetState::ILLEGAL;

	JsonNode presetNode = rewardable->configuration.getPresetVariable(category, name);
	if(presetNode.isNull()) //object is default configured
		return PresetState::VALID;

	auto preset = keyExtractor.filterKeys(presetNode, allowed);
	preset.erase(-1); // remove invalid values
	if(preset.empty())
		return PresetState::INVALID;

	if(keyExtractor.canOverwriteMapSettings(presetNode))
	{
		if(!std::includes(allowed.begin(), allowed.end(), preset.begin(), preset.end()))
			return PresetState::OVERWRITES_MAP_SETTINGS;
	}
	return PresetState::VALID;
}

std::vector<std::string> position(const CGObjectInstance * object)
{
	return {std::to_string(object->pos.x), std::to_string(object->pos.y), std::to_string(object->pos.z)};
}
}

namespace EditorValidator
{

std::set<Issue> validate(const CMap * map)
{
	std::set<Issue> issues;

	auto add = [&issues](const std::string & key, const std::vector<std::string> & arguments, bool critical)
	{
		issues.insert({EditorTexts::format("validator." + key, arguments), critical});
	};

	if(!map)
	{
		add("mapNotLoaded", {}, true);
		return issues;
	}

	try
	{
		//check player settings
		int hplayers = 0;
		int cplayers = 0;
		std::map<PlayerColor, int> amountOfTowns;
		std::map<PlayerColor, int> amountOfHeroes;

		for(size_t i = 0; i < map->players.size(); ++i)
		{
			auto & p = map->players[i];
			if(p.canAnyonePlay())
				amountOfTowns[PlayerColor(static_cast<int>(i))] = 0;
			if(p.canComputerPlay)
				++cplayers;
			if(p.canHumanPlay)
				++hplayers;
			if(p.allowedFactions.empty())
				add("noFactions", {std::to_string(i)}, true);
		}
		if(hplayers + cplayers == 0)
			add("noPlayers", {}, true);
		if(hplayers + cplayers == 1)
			add("onePlayer", {}, true);
		if(!hplayers)
			add("noHuman", {}, true);

		std::set<const CHero *> allHeroesOnMap; //used to find hero duplicated

		//checking all objects in the map
		for(const auto & o : map->objects)
		{
			if(!o)
				continue;

			if(o->isVisitable() && !map->isInTheMap(o->visitablePos()))
				add("visitableOutside", {o->instanceName, o->visitablePos().toString()}, false);

			//owners for objects
			if(o->getOwner() == PlayerColor::UNFLAGGABLE)
			{
				if(o->asOwnable())
					add("unflaggable", {o->instanceName}, true);
			}
			if(o->getOwner() != PlayerColor::NEUTRAL && o->getOwner().getNum() < map->players.size())
			{
				if(!map->players[o->getOwner().getNum()].canAnyonePlay())
					add("nonPlayable", {o->instanceName, o->getOwner().toString()}, true);
			}
			//count towns
			if(auto * ins = dynamic_cast<CGTownInstance *>(o.get()))
			{
				++amountOfTowns[ins->getOwner()];
			}
			//checking and counting heroes and prisons
			if(auto * ins = dynamic_cast<CGHeroInstance *>(o.get()))
			{
				if(ins->ID == Obj::PRISON)
				{
					if(ins->getOwner() != PlayerColor::NEUTRAL)
						add("prisonNeutral", {ins->instanceName}, true);
				}
				else
				{
					if(ins->getOwner() == PlayerColor::NEUTRAL)
						add("heroOwner", {ins->instanceName}, true);

					++amountOfHeroes[ins->getOwner()];
				}
				if(ins->getHeroTypeID().hasValue())
				{
					if(map->allowedHeroes.count(ins->getHeroTypeID()) == 0)
						add("heroProhibited", {ins->getHeroType()->getNameTranslated()}, false);

					if(!allHeroesOnMap.insert(ins->getHeroType()).second)
						add("heroDuplicate", {ins->getHeroType()->getNameTranslated()}, false);
				}
				else if(ins->ID != Obj::RANDOM_HERO)
					add("heroEmpty", {ins->instanceName}, true);
			}

			//checking for arts
			if(auto * ins = dynamic_cast<CGArtifact *>(o.get()))
			{
				if(ins->ID == Obj::SPELL_SCROLL)
				{
					if(ins->getArtifactInstance())
					{
						if(map->allowedSpells.count(ins->getArtifactInstance()->getScrollSpellID()) == 0)
							add("scrollProhibited", {ins->getArtifactInstance()->getScrollSpellID().toEntity(LIBRARY->spells())->getNameTranslated()}, false);
					}
					else
						add("scrollNoInstance", {ins->instanceName}, true);
				}
				else
				{
					if(ins->ID == Obj::ARTIFACT && map->allowedArtifact.count(ins->getArtifactType()) == 0)
						add("artifactProhibited", {EditorTexts::translate(ins->getObjectName())}, false);
				}
			}
			if(o->ID == MapObjectID::WITCH_HUT)
			{
				PresetState presetState = validatePreset(map, o.get(), "secondarySkill", "gainedSkill", map->allowedAbilities);
				if(presetState == PresetState::INVALID)
					add("witchHutInvalid", position(o.get()), true);
				if(presetState == PresetState::ILLEGAL)
					add("witchHutUnverifiable", position(o.get()), false);
			}
			if(o->ID == MapObjectID::SCHOLAR)
			{
				std::pair<PresetState, PresetState> presetStates = {
					validatePreset(map, o.get(), "secondarySkill", "gainedSkill", map->allowedAbilities),
					validatePreset(map, o.get(), "spell", "gainedSpell", map->allowedSpells)
				};
				if(presetStates.first == PresetState::INVALID || presetStates.second == PresetState::INVALID)
					add("scholarInvalid", position(o.get()), true);
				if(presetStates.first == PresetState::OVERWRITES_MAP_SETTINGS || presetStates.second == PresetState::OVERWRITES_MAP_SETTINGS)
					add("scholarOverwrites", position(o.get()), false);
				if(presetStates.first == PresetState::ILLEGAL || presetStates.second == PresetState::ILLEGAL)
					add("scholarUnverifiable", position(o.get()), false);
			}
			static constexpr std::array shrines{MapObjectID::SHRINE_OF_MAGIC_GESTURE, MapObjectID::SHRINE_OF_MAGIC_INCANTATION, MapObjectID::SHRINE_OF_MAGIC_THOUGHT};
			if(vstd::contains(shrines, o->ID))
			{
				PresetState ps = validatePreset(map, o.get(), "spell", "gainedSpell", map->allowedSpells);
				if(ps == PresetState::INVALID)
					add("shrineInvalid", position(o.get()), true);
				if(ps == PresetState::OVERWRITES_MAP_SETTINGS)
					add("shrineOverwrites", position(o.get()), false);
			}
		}

		//verification of starting towns
		for(const auto & [player, counter] : amountOfTowns)
		{
			if(counter == 0)
			{
				if(amountOfHeroes.count(player) == 0)
					add("playerNoTownsHeroes", {std::to_string(player.getNum() + 1)}, true);
				else
					add("playerNoTown", {std::to_string(player.getNum() + 1)}, false);
			}
		}

		//verification of map name and description
		if(map->name.empty())
			add("mapNameMissing", {}, false);
		if(map->description.empty())
			add("mapDescriptionMissing", {}, false);

		//verification for mods
		for(auto & mod : EditorMap::modAssessmentMap(*map))
		{
			if(!map->mods.count(mod.first))
				issues.insert({EditorMap::modMissingMessage(mod.second), true});
		}

		for(const auto & event : map->triggeredEvents)
		{
			auto conditionValidator = [map, &add, &event](const EventCondition & condition) -> EventExpression::Variant
			{
				if(const auto * placeholder = map->isHeroPlaceholderObjective(condition))
				{
					const auto conditionName = EditorTexts::get(event.effect.type == EventEffect::VICTORY ? "validator.conditionDefeatHero" : "validator.conditionLoseHero");
					const std::string placeholderName = placeholder->heroType.has_value()
						? placeholder->heroType->toHeroType()->getNameTranslated()
						: EditorTexts::get("validator.heroPlaceholder");

					add("eventUnusual", {event.identifier, conditionName, placeholderName, condition.position.toString()}, false);
				}

				return condition;
			};

			event.trigger.morph(conditionValidator);
		}
	}
	catch(const std::exception & e)
	{
		add("exception", {e.what()}, true);
	}
	catch(...)
	{
		add("unknownException", {}, true);
	}

	return issues;
}

}

EditorValidatorWindow::EditorValidatorWindow(const CMap * map)
	: CDialogWindow(Point(640, 440), EditorTexts::get("validator.title"), false)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	list = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), 0);

	const auto issues = EditorValidator::validate(map);
	for(const auto & issue : issues)
		list->addParagraph((issue.critical ? EditorTexts::get("validator.criticalPrefix") : EditorTexts::get("validator.warningPrefix")) + " " + issue.message);

	if(issues.empty())
		list->addParagraph(EditorTexts::get("validator.valid"));

	list->finish();
}

void EditorValidatorWindow::show(const CMap * map)
{
	ENGINE->windows().createAndPushWindow<EditorValidatorWindow>(map);
}
