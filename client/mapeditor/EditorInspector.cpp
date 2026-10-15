/*
 * EditorInspector.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorInspector.h"

#include "EditorMap.h"
#include "EditorMapHelpers.h"
#include "EditorTexts.h"

#include "inspector/EditorObjectDialogs.h"
#include "inspector/EditorQuestDialog.h"
#include "inspector/EditorRewardableDialogs.h"
#include "inspector/EditorRewardsDialog.h"
#include "inspector/EditorTownDialogs.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CFormView.h"
#include "../widgets/CTextButton.h"
#include "../widgets/TextControls.h"
#include "../widgets/UiTheme.h"
#include "../windows/CPromptWindows.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/constants/NumericConstants.h"
#include "../../lib/entities/artifact/CArtifactInstance.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/entities/hero/CHeroClass.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/mapObjects/CGDwelling.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGPandoraBox.h"
#include "../../lib/mapObjects/CGResource.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/CRewardableObject.h"
#include "../../lib/mapObjects/FlaggableMapObject.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/mapObjects/Quest.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"

namespace
{
constexpr int HEADER_HEIGHT = 36;
constexpr int PATROL_MAX_RADIUS = 60;
constexpr int MAX_RESOURCE_AMOUNT = 999999;
constexpr int MAX_MINE_PRODUCTION = 99999;
constexpr int MAX_CREATURES = 999999;
constexpr size_t BUTTON_TEXT_LIMIT = 28;

std::string shorten(const std::string & text)
{
	std::string line = text.substr(0, text.find('\n'));
	if(line.size() > BUTTON_TEXT_LIMIT || line.size() < text.size())
	{
		size_t length = std::min(line.size(), BUTTON_TEXT_LIMIT);
		// do not cut an utf-8 sequence in the middle
		while(length > 0 && (line[length] & 0xC0) == 0x80)
			--length;
		line = line.substr(0, length) + "...";
	}
	return line;
}

std::string registerMapText(CMap & map, const TextIdentifier & identifier, const std::string & text)
{
	return mapRegisterLocalizedString("map", map, identifier, text);
}
}

EditorInspector::EditorInspector(const Rect & area, EditorMap & editor)
	: editor(editor)
{
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;

	OBJECT_CONSTRUCTION;
	header = std::make_shared<CLabel>(0, HEADER_HEIGHT / 2, UiTheme::FONT, ETextAlignment::CENTERLEFT, UiTheme::TEXT, "", area.w);
	form = std::make_shared<CFormView>(Rect(0, HEADER_HEIGHT, area.w, area.h - HEADER_HEIGHT), area.w * 2 / 5);
	form->finish();
}

CGObjectInstance * EditorInspector::validObject() const
{
	if(!object || !editor.hasMap())
		return nullptr;

	const auto & objects = editor.getMap()->objects;
	const size_t index = objectId.getNum();
	if(index >= objects.size() || objects[index].get() != object)
		return nullptr;

	return object;
}

void EditorInspector::showObject(CGObjectInstance * newObject)
{
	object = newObject;
	objectId = newObject ? newObject->id : ObjectInstanceID::NONE;
	rebuild();
}

void EditorInspector::clear()
{
	showObject(nullptr);
}

void EditorInspector::refresh()
{
	rebuild();
}

void EditorInspector::setLevel(int newLevel)
{
	level = newLevel;
}

void EditorInspector::changed()
{
	if(onObjectChanged && validObject())
		onObjectChanged(object);
}

void EditorInspector::rebuild()
{
	form->clear();

	CGObjectInstance * o = validObject();
	if(!o)
	{
		object = nullptr;
		header->setText(EditorTexts::get("inspector.nothingSelected"));
		form->finish();
		redraw();
		return;
	}

	header->setText(EditorMapHelpers::describeObject(o));

	form->addInfo(EditorTexts::get("inspector.id"), std::to_string(o->id.getNum()));
	form->addInfo(EditorTexts::get("inspector.type"), std::to_string(o->ID.getNum()) + " / " + std::to_string(o->subID));
	form->addInfo(EditorTexts::get("inspector.instanceName"), o->instanceName);

	if(o->ID != Obj::HERO_PLACEHOLDER && !dynamic_cast<CGHeroInstance *>(o))
	{
		const bool isStatic = LIBRARY->objtypeh->getHandlerFor(o->ID, o->subID)->isStaticObject();
		form->addInfo(EditorTexts::get("inspector.static"), EditorTexts::get(isStatic ? "yes" : "no"));
	}

	const bool ownerEditable = dynamic_cast<CGMine *>(o) || dynamic_cast<CGDwelling *>(o) || dynamic_cast<FlaggableMapObject *>(o)
		|| dynamic_cast<CGGarrison *>(o) || dynamic_cast<CGShipyard *>(o) || dynamic_cast<CGHeroPlaceholder *>(o) || dynamic_cast<CGHeroInstance *>(o);
	const bool prison = o->ID == Obj::PRISON;
	addOwner(EditorTexts::get("inspector.owner"), o->tempOwner, ownerEditable && !prison);

	// the order is from the base classes to the derived ones
	if(auto * p = dynamic_cast<CArmedInstance *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<CGResource *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<CGArtifact *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<CGMine *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<CGGarrison *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<CGDwelling *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<CGTownInstance *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<CGCreature *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<CGHeroPlaceholder *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<CGHeroInstance *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<CGSignBottle *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<CRewardableObject *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<CGPandoraBox *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<CGEvent *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<QuestSource *>(o)) addProperties(p);
	if(auto * p = dynamic_cast<QuestGuard *>(o)) addProperties(p);

	form->finish();
	redraw();
}

// ---------------------------------------------------------------------------------------------
// row helpers

void EditorInspector::addOwner(const std::string & label, PlayerColor & owner, bool editable)
{
	if(!editable)
	{
		form->addInfo(label, owner.isValidPlayer() || owner == PlayerColor::NEUTRAL ? EditorTexts::playerName(owner) : EditorTexts::get("inspector.unflaggable"));
		return;
	}

	auto players = std::make_shared<std::vector<PlayerColor>>();
	std::vector<std::string> names;

	const auto & mapPlayers = editor.getMap()->players;
	for(size_t i = 0; i < mapPlayers.size(); ++i)
	{
		if(mapPlayers[i].canAnyonePlay())
		{
			players->push_back(PlayerColor(i));
			names.push_back(EditorTexts::playerName(PlayerColor(i)));
		}
	}

	const int selected = static_cast<int>(vstd::find_pos(*players, owner));
	PlayerColor * ownerField = &owner;
	form->addChoice(label, names, selected < static_cast<int>(players->size()) ? selected : -1, [this, players, ownerField](int index)
	{
		*ownerField = players->at(index);
		changed();
	});
}

std::shared_ptr<CTextButton> EditorInspector::addTextProperty(const std::string & label, const std::function<std::string()> & getter, const std::function<void(const std::string &)> & setter, bool multiline)
{
	// the callback is owned by the button, so it can not own the button
	auto buttonHolder = std::make_shared<CTextButton *>(nullptr);
	auto button = form->addButton(label, shorten(getter()), [this, buttonHolder, getter, setter, multiline]()
	{
		const auto accepted = [this, buttonHolder, getter, setter](const std::string & text)
		{
			setter(text);
			(*buttonHolder)->setText(shorten(getter()));
			changed();
		};

		if(multiline)
			ENGINE->windows().createAndPushWindow<EditorMessageDialog>(EditorTexts::get("inspector.editText"), getter(), accepted);
		else
			CTextPrompt::show(EditorTexts::get("inspector.editText"), "", getter(), accepted);
	});
	*buttonHolder = button.get();
	return button;
}

void EditorInspector::addNumberProperty(const std::string & label, int value, int minValue, int maxValue, const std::function<void(int)> & setter)
{
	form->addNumber(label, value, minValue, maxValue, [this, setter](int newValue)
	{
		setter(newValue);
		changed();
	});
}

void EditorInspector::addCheckProperty(const std::string & label, bool value, const std::function<void(bool)> & setter)
{
	form->addCheck(label, value, [this, setter](bool on)
	{
		setter(on);
		changed();
	});
}

void EditorInspector::addChoiceProperty(const std::string & label, const std::vector<std::string> & options, int selected, const std::function<void(int)> & setter)
{
	form->addChoice(label, options, selected, [this, setter](int index)
	{
		setter(index);
		changed();
	});
}

void EditorInspector::addDialogProperty(const std::string & label, const std::string & buttonText, const std::function<void()> & opener)
{
	form->addButton(label, buttonText, opener);
}

// ---------------------------------------------------------------------------------------------
// properties of the object kinds

void EditorInspector::addProperties(CArmedInstance * o)
{
	addDialogProperty(EditorTexts::get("inspector.army"), EditorTexts::get("inspector.edit"), [this, o]()
	{
		ENGINE->windows().createAndPushWindow<EditorArmyDialog>(*o, [this]()
		{
			changed();
			rebuild();
		});
	});
}

void EditorInspector::addProperties(CGResource * o)
{
	addNumberProperty(EditorTexts::get("inspector.amount"), static_cast<int>(o->getAmount()), 0, MAX_RESOURCE_AMOUNT, [o](int value)
	{
		o->setAmount(value);
	});
	form->addInfo(EditorTexts::get("inspector.message"), shorten(EditorTexts::translate(o->getMessage())));
}

void EditorInspector::addProperties(CGArtifact * o)
{
	addTextProperty(EditorTexts::get("inspector.message"), [o]() { return EditorTexts::translate(o->message); }, [this, o](const std::string & text)
	{
		o->message = MetaString::createFromTextID(registerMapText(*editor.getMap(), TextIdentifier("guards", o->instanceName, "message"), text));
	}, true);

	if(MapObjectID::isRandomArtifact(o->ID) || o->ID != Obj::SPELL_SCROLL)
		return;

	const CArtifactInstance * instance = o->getArtifactInstance();
	if(!instance || instance->getScrollSpellID() == SpellID::NONE)
		return;

	auto spells = std::make_shared<std::vector<SpellID>>();
	std::vector<std::string> names;
	for(const auto & spell : LIBRARY->spellh->objects)
	{
		if(vstd::contains(editor.getMap()->allowedSpells, spell->getId()))
		{
			spells->push_back(spell->getId());
			names.push_back(spell->getNameTranslated());
		}
	}

	addChoiceProperty(EditorTexts::get("inspector.spell"), names, static_cast<int>(vstd::find_pos(*spells, instance->getScrollSpellID())), [this, o, spells](int index)
	{
		o->setArtifactInstance(editor.getMap()->createScroll(spells->at(index)));
	});
}

void EditorInspector::addProperties(CGMine * o)
{
	if(o->producedResource != GameResID::NONE)
	{
		MetaString resource;
		resource.appendName(o->producedResource);
		form->addInfo(EditorTexts::get("inspector.resource"), EditorTexts::translate(resource));
	}

	addNumberProperty(EditorTexts::get("inspector.productivity"), static_cast<int>(o->producedQuantity), 0, MAX_MINE_PRODUCTION, [o](int value)
	{
		o->producedQuantity = value;
	});
}

void EditorInspector::addProperties(CGGarrison * o)
{
	addCheckProperty(EditorTexts::get("inspector.removableUnits"), o->removableUnits, [o](bool on)
	{
		o->removableUnits = on;
	});
}

void EditorInspector::addProperties(CGDwelling * o)
{
	if(o->ID != Obj::RANDOM_DWELLING && o->ID != Obj::RANDOM_DWELLING_LVL)
		return;

	const std::string current = o->randomizationInfo.has_value() && !o->randomizationInfo->instanceId.empty() ? o->randomizationInfo->instanceId : EditorTexts::get("inspector.selectTown");
	form->addButton(EditorTexts::get("inspector.sameAsTown"), current, [this, o]()
	{
		const auto filter = [](const CGObjectInstance * candidate) { return dynamic_cast<const CGTownInstance *>(candidate) != nullptr; };
		EditorMapHelpers::pickObject(editor, 0, filter, [this, o](const CGObjectInstance * picked)
		{
			if(!o->randomizationInfo.has_value())
				o->randomizationInfo = CGDwellingRandomizationInfo();

			o->randomizationInfo->instanceId = picked ? picked->instanceName : "";
			changed();
			rebuild();
		});
	});
}

void EditorInspector::addProperties(CGTownInstance * o)
{
	addTextProperty(EditorTexts::get("inspector.townName"), [o]() { return EditorTexts::translate(TextIdentifier(o->getNameTextID())); }, [this, o](const std::string & text)
	{
		o->setNameTextId(registerMapText(*editor.getMap(), TextIdentifier("town", o->instanceName, "name"), text));
	}, false);

	addDialogProperty(EditorTexts::get("inspector.buildings"), EditorTexts::get("inspector.edit"), [this, o]()
	{
		ENGINE->windows().createAndPushWindow<EditorTownBuildingsDialog>(*o, [this]() { changed(); });
	});
	addDialogProperty(EditorTexts::get("inspector.spells"), EditorTexts::get("inspector.edit"), [this, o]()
	{
		ENGINE->windows().createAndPushWindow<EditorTownSpellsDialog>(*o, [this]() { changed(); });
	});
	addDialogProperty(EditorTexts::get("inspector.events"), EditorTexts::get("inspector.edit"), [this, o]()
	{
		ENGINE->windows().createAndPushWindow<EditorTownEventsDialog>(editor, *o, [this]() { changed(); });
	});

	if(o->ID == Obj::RANDOM_TOWN)
		addOwner(EditorTexts::get("inspector.sameAsPlayer"), o->alignmentToPlayer, true);
}

void EditorInspector::addProperties(CGCreature * o)
{
	addTextProperty(EditorTexts::get("inspector.message"), [o]() { return EditorTexts::translate(o->message); }, [this, o](const std::string & text)
	{
		o->message = MetaString::createFromTextID(registerMapText(*editor.getMap(), TextIdentifier("monster", o->instanceName, "message"), text));
	}, true);

	using Character = CGCreature::Character;
	auto characters = std::make_shared<std::vector<Character>>(std::vector<Character>{Character::COMPLIANT, Character::FRIENDLY, Character::AGGRESSIVE, Character::HOSTILE, Character::SAVAGE, Character::CUSTOM});
	addChoiceProperty(EditorTexts::get("inspector.character"),
		{EditorTexts::get("character.compliant"), EditorTexts::get("character.friendly"), EditorTexts::get("character.aggressive"), EditorTexts::get("character.hostile"), EditorTexts::get("character.savage"), EditorTexts::get("character.custom")},
		static_cast<int>(vstd::find_pos(*characters, o->initialCharacter)), [o, characters](int index)
		{
			o->initialCharacter = characters->at(index);
		});

	addCheckProperty(EditorTexts::get("inspector.neverFlees"), o->neverFlees, [o](bool on) { o->neverFlees = on; });
	addCheckProperty(EditorTexts::get("inspector.notGrowing"), o->notGrowingTeam, [o](bool on) { o->notGrowingTeam = on; });

	// only the custom character keeps the aggression of the map, the others derive it on map start
	addNumberProperty(EditorTexts::get("inspector.aggression"), o->agression, -1, 10, [o](int value) { o->agression = static_cast<int8_t>(value); });
	addCheckProperty(EditorTexts::get("inspector.joinOnlyForMoney"), o->joinOnlyForMoney, [o](bool on) { o->joinOnlyForMoney = on; });
	// -1 uses the global setting
	addNumberProperty(EditorTexts::get("inspector.joiningPercentage"), o->joiningPercentage, -1, 100, [o](int value) { o->joiningPercentage = static_cast<int8_t>(value); });

	using Presence = CGCreature::UpgradedStackPresence;
	auto presences = std::make_shared<std::vector<Presence>>(std::vector<Presence>{Presence::RANDOM, Presence::NEVER, Presence::ALWAYS});
	addChoiceProperty(EditorTexts::get("inspector.upgradedStack"), {EditorTexts::get("presence.random"), EditorTexts::get("presence.never"), EditorTexts::get("presence.always")},
		static_cast<int>(vstd::find_pos(*presences, o->upgradedStackPresence)), [o, presences](int index)
		{
			o->upgradedStackPresence = presences->at(index);
		});

	// the negative values are the relative codes of the HotA format: -1 default, -2 one less, -3 average, 0 one more
	addNumberProperty(EditorTexts::get("inspector.stacksCount"), static_cast<int>(o->stacksCount), -3, GameConstants::ARMY_SIZE, [o](int value) { o->stacksCount = value; });

	if(o->hasStackAtSlot(SlotID(0)))
	{
		addNumberProperty(EditorTexts::get("inspector.amount"), o->getStackCount(SlotID(0)), 1, MAX_CREATURES, [o](int value)
		{
			o->stacks.at(SlotID(0))->setCount(value);
		});
	}
}

void EditorInspector::addProperties(CGHeroPlaceholder * o)
{
	const bool byType = o->heroType.has_value();

	std::vector<std::string> heroNames;
	for(const auto & hero : LIBRARY->heroh->objects)
		heroNames.push_back(hero->getNameTranslated());

	addChoiceProperty(EditorTexts::get("inspector.placeholderType"), {EditorTexts::get("inspector.powerRank"), EditorTexts::get("inspector.heroType")}, byType ? 1 : 0, [this, o](int index)
	{
		if(index == 1)
		{
			if(!o->heroType.has_value())
				o->heroType = HeroTypeID(0);
			o->powerRank.reset();
		}
		else
		{
			if(!o->powerRank.has_value())
				o->powerRank = 0;
			o->heroType.reset();
		}
		rebuild();
	});

	if(byType)
	{
		addChoiceProperty(EditorTexts::get("inspector.heroType"), heroNames, o->heroType->getNum(), [o](int index)
		{
			o->heroType = HeroTypeID(index);
		});
	}
	else
	{
		addNumberProperty(EditorTexts::get("inspector.powerRank"), o->powerRank.value_or(0), 0, 255, [o](int value)
		{
			o->powerRank = static_cast<ui8>(value);
		});
	}
}

void EditorInspector::addProperties(CGHeroInstance * o)
{
	const bool prison = o->ID == Obj::PRISON;

	addNumberProperty(EditorTexts::get("inspector.experience"), static_cast<int>(o->exp), 0, std::numeric_limits<int>::max(), [o](int value)
	{
		o->exp = value;
	});
	form->addInfo(EditorTexts::get("inspector.heroClass"), o->getHeroClassID().hasValue() ? o->getHeroClass()->getNameTranslated() : "");

	auto genders = std::make_shared<std::vector<EHeroGender>>(std::vector<EHeroGender>{EHeroGender::MALE, EHeroGender::FEMALE});
	addChoiceProperty(EditorTexts::get("inspector.gender"), {EditorTexts::get("gender.male"), EditorTexts::get("gender.female")}, o->gender == EHeroGender::FEMALE ? 1 : 0, [o, genders](int index)
	{
		o->gender = genders->at(index);
	});

	addTextProperty(EditorTexts::get("inspector.name"), [o]() { return EditorTexts::translate(TextIdentifier(o->getNameTextID())); }, [this, o](const std::string & text)
	{
		o->nameCustomTextId = registerMapText(*editor.getMap(), TextIdentifier("hero", o->instanceName, "name"), text);
	}, false);
	addTextProperty(EditorTexts::get("inspector.biography"), [o]() { return EditorTexts::translate(TextIdentifier(o->getBiographyTextID())); }, [this, o](const std::string & text)
	{
		o->biographyCustomTextId = registerMapText(*editor.getMap(), TextIdentifier("hero", o->instanceName, "biography"), text);
	}, true);

	addDialogProperty(EditorTexts::get("inspector.portrait"), EditorTexts::get("inspector.edit"), [this, o]()
	{
		ENGINE->windows().createAndPushWindow<EditorPortraitDialog>(*o, [this]() { changed(); });
	});
	addDialogProperty(EditorTexts::get("inspector.skills"), EditorTexts::get("inspector.edit"), [this, o]()
	{
		ENGINE->windows().createAndPushWindow<EditorHeroSkillsDialog>(*o, [this]() { changed(); });
	});
	addDialogProperty(EditorTexts::get("inspector.spells"), EditorTexts::get("inspector.edit"), [this, o]()
	{
		ENGINE->windows().createAndPushWindow<EditorHeroSpellsDialog>(*o, [this]() { changed(); });
	});
	addDialogProperty(EditorTexts::get("inspector.artifacts"), EditorTexts::get("inspector.edit"), [this, o]()
	{
		ENGINE->windows().createAndPushWindow<EditorHeroArtifactsDialog>(editor, *o, [this]() { changed(); });
	});

	if(o->getHeroTypeID().hasValue() || prison)
	{
		auto heroes = std::make_shared<std::vector<int>>();
		std::vector<std::string> names;
		for(const auto & hero : LIBRARY->heroh->objects)
		{
			if(!vstd::contains(editor.getMap()->allowedHeroes, hero->getId()))
				continue;

			if(prison || hero->heroClass->getIndex() == o->getHeroClassID())
			{
				heroes->push_back(hero->getIndex());
				names.push_back(hero->getNameTranslated());
			}
		}

		const int current = o->getHeroTypeID().hasValue() ? static_cast<int>(vstd::find_pos(*heroes, o->getHeroTypeID().getNum())) : -1;
		addChoiceProperty(EditorTexts::get("inspector.heroType"), names, current < static_cast<int>(heroes->size()) ? current : -1, [this, o, heroes](int index)
		{
			const HeroTypeID type(heroes->at(index));
			o->subID = type.getNum();
			o->gender = o->getHeroType()->gender;
			o->randomizeArmy(o->getHeroType()->heroClass->faction);
			changed();
			rebuild();
		});
	}

	std::vector<std::string> patrolOptions{EditorTexts::get("inspector.noPatrol")};
	for(int i = 0; i <= PATROL_MAX_RADIUS; ++i)
		patrolOptions.push_back(EditorTexts::format("inspector.tiles", {std::to_string(i)}));

	const int patrolSelected = o->patrol.patrolling ? static_cast<int>(o->patrol.patrolRadius) + 1 : 0;
	addChoiceProperty(EditorTexts::get("inspector.patrolRadius"), patrolOptions, patrolSelected, [o](int index)
	{
		o->patrol.patrolling = index > 0;
		o->patrol.patrolRadius = index > 0 ? index - 1 : CGHeroInstance::NO_PATROLLING;
	});
}

void EditorInspector::addProperties(CGSignBottle * o)
{
	addTextProperty(EditorTexts::get("inspector.message"), [o]() { return EditorTexts::translate(o->message); }, [this, o](const std::string & text)
	{
		o->message = MetaString::createFromTextID(registerMapText(*editor.getMap(), TextIdentifier("sign", o->instanceName, "message"), text));
	}, true);
}

void EditorInspector::addProperties(CRewardableObject * o)
{
	std::function<void(const std::function<void()> &)> opener;
	switch(o->ID.getNum())
	{
		case MapObjectID::WITCH_HUT:
			opener = [this, o](const std::function<void()> & done)
			{
				ENGINE->windows().createAndPushWindow<EditorAbilitiesDialog>(editor, *o, done);
			};
			break;
		case MapObjectID::SCHOLAR:
			opener = [o](const std::function<void()> & done)
			{
				ENGINE->windows().createAndPushWindow<EditorScholarDialog>(*o, done);
			};
			break;
		case MapObjectID::SHRINE_OF_MAGIC_INCANTATION:
		case MapObjectID::SHRINE_OF_MAGIC_GESTURE:
		case MapObjectID::SHRINE_OF_MAGIC_THOUGHT:
			opener = [o](const std::function<void()> & done)
			{
				EditorShrineDialog::show(*o, done);
			};
			break;
		default:
			opener = [this, o](const std::function<void()> & done)
			{
				ENGINE->windows().createAndPushWindow<EditorRewardsDialog>(editor, *o, done);
			};
	}

	addDialogProperty(EditorTexts::get("inspector.reward"), EditorTexts::get("inspector.edit"), [this, opener]()
	{
		opener([this]() { changed(); });
	});
}

void EditorInspector::addProperties(CGPandoraBox * o)
{
	addTextProperty(EditorTexts::get("inspector.message"), [o]() { return EditorTexts::translate(o->message); }, [this, o](const std::string & text)
	{
		o->message = MetaString::createFromTextID(registerMapText(*editor.getMap(), TextIdentifier("guards", o->instanceName, "message"), text));
	}, true);
}

void EditorInspector::addProperties(CGEvent * o)
{
	addCheckProperty(EditorTexts::get("inspector.removeAfter"), o->removeAfterVisit, [o](bool on) { o->removeAfterVisit = on; });
	addCheckProperty(EditorTexts::get("inspector.humanTrigger"), o->humanActivate, [o](bool on) { o->humanActivate = on; });
	addCheckProperty(EditorTexts::get("inspector.cpuTrigger"), o->computerActivate, [o](bool on) { o->computerActivate = on; });

	addDialogProperty(EditorTexts::get("inspector.availableFor"), EditorTexts::get("inspector.edit"), [this, o]()
	{
		ENGINE->windows().createAndPushWindow<EditorPlayerSetDialog>(EditorTexts::get("inspector.availableFor"), o->availableFor, [this, o](const std::set<PlayerColor> & players)
		{
			o->availableFor = players;
			changed();
		});
	});
}

void EditorInspector::addProperties(QuestSource * o)
{
	addDialogProperty(EditorTexts::get("inspector.quest"), EditorTexts::get("inspector.edit"), [this, o]()
	{
		ENGINE->windows().createAndPushWindow<EditorQuestDialog>(editor, *o, [this]() { changed(); });
	});
}

void EditorInspector::addProperties(QuestGuard * o)
{
	form->addInfo(EditorTexts::get("inspector.repeatQuest"), EditorTexts::get(o->getQuest().repeatedQuest ? "yes" : "no"));
}
