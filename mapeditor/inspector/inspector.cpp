/*
 * inspector.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "inspector.h"
#include "../lib/ArtifactUtils.h"
#include "../lib/CArtHandler.h"
#include "../lib/entities/hero/CHeroClass.h"
#include "../lib/entities/hero/CHeroHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../lib/mapObjects/ObjectTemplate.h"
#include "../lib/mapping/CMap.h"
#include "../lib/constants/StringConstants.h"

#include "townbuildingswidget.h"
#include "towneventswidget.h"
#include "townspellswidget.h"
#include "armywidget.h"
#include "messagewidget.h"
#include "rewardswidget.h"
#include "questwidget.h"
#include "heroartifactswidget.h"
#include "heroskillswidget.h"
#include "herospellwidget.h"
#include "portraitwidget.h"
#include "PickObjectDelegate.h"
#include "../mapcontroller.h"

static QList<std::pair<QString, QVariant>> CharacterIdentifiers
{
	{QObject::tr("Compliant"), QVariant::fromValue(int(CGCreature::Character::COMPLIANT))},
	{QObject::tr("Friendly"), QVariant::fromValue(int(CGCreature::Character::FRIENDLY))},
	{QObject::tr("Aggressive"), QVariant::fromValue(int(CGCreature::Character::AGGRESSIVE))},
	{QObject::tr("Hostile"), QVariant::fromValue(int(CGCreature::Character::HOSTILE))},
	{QObject::tr("Savage"), QVariant::fromValue(int(CGCreature::Character::SAVAGE))},
};

// multiple times used translation strings
enum ReusedKey { ARMY, OWNER, SAME_AS_TOWN, REMOVABLE_UNITS, PLACEHOLDER_TYPE, POWER_RANK, HERO_TYPE, EXPERIENCE, MALE, FEMALE, GENDER, NAME, BIOGRAPHY, SPELLS, NO_PATROL, PATROL_RADIUS, TOWN_NAME, MESSAGE, SPELL, PRODUCTIVITY, AMOUNT, CHARACTER, NEVER_FLEES, NOT_GROWING, REWARD, REMOVE_AFTER, HUMAN_TRIGGER, CPU_TRIGGER, FIRST_VISIT_TEXT, NEXT_VISIT_TEXT, COMPLETED_TEXT, REPEAT_QUEST, TIME_LIMIT };
static const QHash<ReusedKey, QString> trReused =
{
	{ ARMY, QObject::tr("Army") },
	{ OWNER, QObject::tr("Owner") },
	{ SAME_AS_TOWN, QObject::tr("Same as town") },
	{ REMOVABLE_UNITS, QObject::tr("Removable units") },
	{ PLACEHOLDER_TYPE, QObject::tr("Placeholder type") },
	{ POWER_RANK, QObject::tr("Power rank") },
	{ HERO_TYPE, QObject::tr("Hero type") },
	{ EXPERIENCE, QObject::tr("Experience") },
	{ MALE, QObject::tr("MALE") },
	{ FEMALE, QObject::tr("FEMALE") },
	{ GENDER, QObject::tr("Gender") },
	{ NAME, QObject::tr("Name") },
	{ BIOGRAPHY, QObject::tr("Biography") },
	{ SPELLS, QObject::tr("Spells") },
	{ NO_PATROL, QObject::tr("No patrol") },
	{ PATROL_RADIUS, QObject::tr("Patrol radius") },
	{ TOWN_NAME, QObject::tr("Town name") },
	{ MESSAGE, QObject::tr("Message") },
	{ SPELL, QObject::tr("Spell") },
	{ PRODUCTIVITY, QObject::tr("Productivity") },
	{ AMOUNT, QObject::tr("Amount") },
	{ CHARACTER, QObject::tr("Character") },
	{ NEVER_FLEES, QObject::tr("Never flees") },
	{ NOT_GROWING, QObject::tr("Not growing") },
	{ REWARD, QObject::tr("Reward") },
	{ REMOVE_AFTER, QObject::tr("Remove after") },
	{ HUMAN_TRIGGER, QObject::tr("Human trigger") },
	{ CPU_TRIGGER, QObject::tr("Cpu trigger") },
	{ FIRST_VISIT_TEXT, QObject::tr("First visit text") },
	{ NEXT_VISIT_TEXT, QObject::tr("Next visit text") },
	{ COMPLETED_TEXT, QObject::tr("Completed text") },
	{ REPEAT_QUEST, QObject::tr("Repeat quest") },
	{ TIME_LIMIT, QObject::tr("Time limit") },
};

//===============IMPLEMENT OBJECT INITIALIZATION FUNCTIONS================
Initializer::Initializer(CGObjectInstance * o, const PlayerColor & pl) : defaultPlayer(pl)
{
	logGlobal->info("New object instance initialized");
///IMPORTANT! initialize order should be from base objects to derived objects
	INIT_OBJ_TYPE(CGResource);
	INIT_OBJ_TYPE(CGArtifact);
	INIT_OBJ_TYPE(CArmedInstance);
	INIT_OBJ_TYPE(CGShipyard);
	INIT_OBJ_TYPE(CGGarrison);
	INIT_OBJ_TYPE(CGMine);
	INIT_OBJ_TYPE(CGDwelling);
	INIT_OBJ_TYPE(CGTownInstance);
	INIT_OBJ_TYPE(CGCreature);
	INIT_OBJ_TYPE(CGHeroPlaceholder);
	INIT_OBJ_TYPE(CGHeroInstance);
	INIT_OBJ_TYPE(CGSignBottle);
	INIT_OBJ_TYPE(FlaggableMapObject);
	//INIT_OBJ_TYPE(CRewardableObject);
	//INIT_OBJ_TYPE(CGPandoraBox);
	//INIT_OBJ_TYPE(CGEvent);
	//INIT_OBJ_TYPE(CGSeerHut);
}

void Initializer::initialize(CArmedInstance * o)
{
	if(!o) return;
}

void Initializer::initialize(CGSignBottle * o)
{
	if(!o) return;
}

void Initializer::initialize(CGCreature * o)
{
	if(!o) return;
	
	o->character = CGCreature::Character::HOSTILE;
	if(!o->hasStackAtSlot(SlotID(0)))
	   o->putStack(SlotID(0), new CStackInstance(CreatureID(o->subID), 0, false));
}

void Initializer::initialize(CGDwelling * o)
{
	if(!o) return;
	
	o->tempOwner = defaultPlayer;

	if(o->ID == Obj::RANDOM_DWELLING || o->ID == Obj::RANDOM_DWELLING_LVL || o->ID == Obj::RANDOM_DWELLING_FACTION)
	{
		o->randomizationInfo = CGDwellingRandomizationInfo();
		if(o->ID == Obj::RANDOM_DWELLING_LVL)
		{
			o->randomizationInfo->minLevel = o->subID;
			o->randomizationInfo->maxLevel = o->subID;
		}
		if(o->ID == Obj::RANDOM_DWELLING_FACTION)
		{
			o->randomizationInfo->allowedFactions.insert(FactionID(o->subID));
		}
	}
}

void Initializer::initialize(CGGarrison * o)
{
	if(!o) return;
	
	o->tempOwner = defaultPlayer;
	o->removableUnits = true;
}

void Initializer::initialize(CGShipyard * o)
{
	if(!o) return;
	
	o->tempOwner = defaultPlayer;
}

void Initializer::initialize(FlaggableMapObject * o)
{
	if(!o) return;
	
	o->tempOwner = defaultPlayer;
}

void Initializer::initialize(CGHeroPlaceholder * o)
{
	if(!o) return;
	
	o->tempOwner = defaultPlayer;
	
	if(!o->powerRank.has_value() && !o->heroType.has_value())
		o->powerRank = 0;
	
	if(o->powerRank.has_value() && o->heroType.has_value())
		o->powerRank.reset();
}

void Initializer::initialize(CGHeroInstance * o)
{
	if(!o)
		return;
	
	o->tempOwner = defaultPlayer;
	if(o->ID == Obj::PRISON)
	{
		o->subID = 0;
		o->tempOwner = PlayerColor::NEUTRAL;
	}

	if(o->ID == Obj::HERO)
	{
		for(auto const & t : VLC->heroh->objects)
		{
			if(t->heroClass->getId() == HeroClassID(o->subID))
			{
				o->subID = t->getId();
				break;
			}
		}
	}

	if(o->getHeroTypeID().hasValue())
	{
		o->gender = o->getHeroType()->gender;
		o->randomizeArmy(o->getFactionID());
	}
}

void Initializer::initialize(CGTownInstance * o)
{
	if(!o) return;

	const std::vector<std::string> castleLevels{"village", "fort", "citadel", "castle", "capitol"};
	int lvl = vstd::find_pos(castleLevels, o->appearance->stringID);
	o->addBuilding(BuildingID::DEFAULT);
	if(lvl > -1) o->addBuilding(BuildingID::TAVERN);
	if(lvl > 0) o->addBuilding(BuildingID::FORT);
	if(lvl > 1) o->addBuilding(BuildingID::CITADEL);
	if(lvl > 2) o->addBuilding(BuildingID::CASTLE);
	if(lvl > 3) o->addBuilding(BuildingID::CAPITOL);

	if(o->possibleSpells.empty())
	{
		for(auto const & spellId : VLC->spellh->getDefaultAllowed()) //add all regular spells to town
		{
			o->possibleSpells.push_back(spellId);
		}
	}
}

void Initializer::initialize(CGArtifact * o)
{
	if(!o) return;
	
	if(o->ID == Obj::SPELL_SCROLL)
	{
		std::vector<SpellID> out;
		for(auto const & spell : VLC->spellh->objects) //spellh size appears to be greater (?)
		{
			if(VLC->spellh->getDefaultAllowed().count(spell->id) != 0)
			{
				out.push_back(spell->id);
			}
		}
		auto a = ArtifactUtils::createScroll(*RandomGeneratorUtil::nextItem(out, CRandomGenerator::getDefault()));
		o->storedArtifact = a;
	}
}

void Initializer::initialize(CGMine * o)
{
	if(!o) return;
	
	o->tempOwner = defaultPlayer;
	if(o->isAbandoned())
	{
		for(auto r = GameResID(0); r < GameResID::COUNT; ++r)
			o->abandonedMineResources.insert(r);
	}
	else
	{
		o->producedResource = GameResID(o->subID);
		o->producedQuantity = o->defaultResProduction();
	}
}

void Initializer::initialize(CGResource * o)
{
	if(!o) return;
	
	o->amount = CGResource::RANDOM_AMOUNT;
}

//===============IMPLEMENT PROPERTIES SETUP===============================
void Inspector::updateProperties(CArmedInstance * o)
{
	if(!o) return;
	
	auto * delegate = new ArmyDelegate(*o);
	addProperty(trReused[ARMY], PropertyEditorPlaceholder(), delegate, false);
}

void Inspector::updateProperties(CGDwelling * o)
{
	if(!o) return;
	
	addProperty(trReused[OWNER], o->tempOwner, new OwnerDelegate(controller), false);
	
	if (o->ID == Obj::RANDOM_DWELLING || o->ID == Obj::RANDOM_DWELLING_LVL)
	{
		auto * delegate = new PickObjectDelegate(controller, PickObjectDelegate::typedFilter<CGTownInstance>);
		addProperty(trReused[SAME_AS_TOWN], PropertyEditorPlaceholder(), delegate, false);
	}
}

void Inspector::updateProperties(FlaggableMapObject * o)
{
	if(!o) return;

	addProperty(trReused[OWNER], o->tempOwner, new OwnerDelegate(controller), false);
}

void Inspector::updateProperties(CGGarrison * o)
{
	if(!o) return;
	
	addProperty(trReused[OWNER], o->tempOwner, new OwnerDelegate(controller), false);
	addProperty(trReused[REMOVABLE_UNITS], o->removableUnits, false);
}

void Inspector::updateProperties(CGShipyard * o)
{
	if(!o) return;
	
	addProperty(trReused[OWNER], o->tempOwner, new OwnerDelegate(controller), false);
}

void Inspector::updateProperties(CGHeroPlaceholder * o)
{
	if(!o) return;
	
	addProperty(trReused[OWNER], o->tempOwner, new OwnerDelegate(controller), false);
	
	bool type = false;
	if(o->heroType.has_value())
		type = true;
	else if(!o->powerRank.has_value())
		assert(0); //one of values must be initialized
	
	{
		auto * delegate = new InspectorDelegate;
		delegate->options = {{QObject::tr("POWER RANK"), QVariant::fromValue(false)}, {QObject::tr("HERO TYPE"), QVariant::fromValue(true)}};
		addProperty(trReused[PLACEHOLDER_TYPE], delegate->options[type].first, delegate, false);
	}
	
	addProperty(trReused[POWER_RANK], o->powerRank.has_value() ? o->powerRank.value() : 0, type);
	
	{
		auto * delegate = new InspectorDelegate;
		for(int i = 0; i < VLC->heroh->objects.size(); ++i)
		{
			delegate->options.push_back({QObject::tr(VLC->heroh->objects[i]->getNameTranslated().c_str()), QVariant::fromValue(VLC->heroh->objects[i]->getId().getNum())});
		}
		addProperty(trReused[HERO_TYPE], o->heroType.has_value() ? VLC->heroh->getById(o->heroType.value())->getNameTranslated() : "", delegate, !type);
	}
}

void Inspector::updateProperties(CGHeroInstance * o)
{
	if(!o) return;
	
	auto isPrison = o->ID == Obj::PRISON;
	addProperty(trReused[OWNER], o->tempOwner, new OwnerDelegate(controller, isPrison), isPrison); //field is not editable for prison
	addProperty<int>(trReused[EXPERIENCE], o->exp, false);
	addProperty(QObject::tr("Hero class"), o->getHeroClassID().hasValue() ? o->getHeroClass()->getNameTranslated() : "", true);
	
	{ //Gender
		auto * delegate = new InspectorDelegate;
		delegate->options = {{trReused[MALE], QVariant::fromValue(int(EHeroGender::MALE))}, {trReused[FEMALE], QVariant::fromValue(int(EHeroGender::FEMALE))}};
		addProperty<std::string>(trReused[GENDER], (o->gender == EHeroGender::FEMALE ? trReused[FEMALE] : trReused[MALE]).toStdString(), delegate , false);
	}
	addProperty(trReused[NAME], o->getNameTranslated(), false);
	addProperty(trReused[BIOGRAPHY], o->getBiographyTranslated(), new MessageDelegate, false);
	addProperty(QObject::tr("Portrait"), PropertyEditorPlaceholder(), new PortraitDelegate(*o), false);
	
	auto * delegate = new HeroSkillsDelegate(*o);
	addProperty(QObject::tr("Skills"), PropertyEditorPlaceholder(), delegate, false);
	addProperty(trReused[SPELLS], PropertyEditorPlaceholder(), new HeroSpellDelegate(*o), false);
	addProperty(QObject::tr("Artifacts"), PropertyEditorPlaceholder(), new HeroArtifactsDelegate(*o), false);
	
	if(o->getHeroTypeID().hasValue() || o->ID == Obj::PRISON)
	{ //Hero type
		auto * delegate = new InspectorDelegate;
		for(const auto & heroPtr : VLC->heroh->objects)
		{
			if(controller.map()->allowedHeroes.count(heroPtr->getId()) != 0)
			{
				if(o->ID == Obj::PRISON || heroPtr->heroClass->getIndex() == o->getHeroClassID())
				{
					delegate->options.push_back({QObject::tr(heroPtr->getNameTranslated().c_str()), QVariant::fromValue(heroPtr->getIndex())});
				}
			}
		}
		addProperty(trReused[HERO_TYPE], o->getHeroTypeID().hasValue() ? o->getHeroType()->getNameTranslated() : "", delegate, false);
	}
	{
		const int maxRadius = 60;
		auto * patrolDelegate = new InspectorDelegate;
		patrolDelegate->options = { {QObject::tr("No patrol"), QVariant::fromValue(CGHeroInstance::NO_PATROLLING)} };
		for(int i = 0; i <= maxRadius; ++i)
			patrolDelegate->options.push_back({ QObject::tr("%n tile(s)", "", i), QVariant::fromValue(i)});
		auto patrolRadiusText = o->patrol.patrolling ? QObject::tr("%n tile(s)", "", o->patrol.patrolRadius) : QObject::tr("No patrol");
		addProperty(trReused[PATROL_RADIUS], patrolRadiusText, patrolDelegate, false);
	}
}

void Inspector::updateProperties(CGTownInstance * o)
{
	if(!o) return;
	
	addProperty(trReused[TOWN_NAME], o->getNameTranslated(), false);
	
	auto * delegate = new TownBuildingsDelegate(*o);
	addProperty(QObject::tr("Buildings"), PropertyEditorPlaceholder(), delegate, false);
	addProperty(trReused[SPELLS], PropertyEditorPlaceholder(), new TownSpellsDelegate(*o), false);
	addProperty(QObject::tr("Events"), PropertyEditorPlaceholder(), new TownEventsDelegate(*o, controller), false);
}

void Inspector::updateProperties(CGArtifact * o)
{
	if(!o) return;
	
	addProperty(trReused[MESSAGE], o->message, false);
	
	CArtifactInstance * instance = o->storedArtifact;
	if(instance)
	{
		SpellID spellId = instance->getScrollSpellID();
		if(spellId != SpellID::NONE)
		{
			auto * delegate = new InspectorDelegate;
			for(auto const & spell : VLC->spellh->objects)
			{
				if(controller.map()->allowedSpells.count(spell->id) != 0)
					delegate->options.push_back({QObject::tr(spell->getNameTranslated().c_str()), QVariant::fromValue(int(spell->getId()))});
			}
			addProperty(trReused[SPELL], VLC->spellh->getById(spellId)->getNameTranslated(), delegate, false);
		}
	}
}

void Inspector::updateProperties(CGMine * o)
{
	if(!o) return;
	
	addProperty(trReused[OWNER], o->tempOwner, new OwnerDelegate(controller), false);
	addProperty(QObject::tr("Resource"), o->producedResource);
	addProperty(trReused[PRODUCTIVITY], o->producedQuantity);
}

void Inspector::updateProperties(CGResource * o)
{
	if(!o) return;
	
	addProperty(trReused[AMOUNT], o->amount, false);
	addProperty(trReused[MESSAGE], o->message, false);
}

void Inspector::updateProperties(CGSignBottle * o)
{
	if(!o) return;
	
	addProperty(trReused[MESSAGE], o->message, new MessageDelegate, false);
}

void Inspector::updateProperties(CGCreature * o)
{
	if(!o) return;
	
	addProperty(trReused[MESSAGE], o->message, false);
	{ //Character
		auto * delegate = new InspectorDelegate;
		delegate->options = CharacterIdentifiers;
		addProperty<CGCreature::Character>("Character", (CGCreature::Character)o->character, delegate, false);
	}
	addProperty(trReused[NEVER_FLEES], o->neverFlees, false);
	addProperty(trReused[NOT_GROWING], o->notGrowingTeam, false);
	addProperty(QObject::tr("Artifact reward"), o->gainedArtifact); //TODO: implement in setProperty
	addProperty(trReused[ARMY], PropertyEditorPlaceholder(), true);
	addProperty(trReused[AMOUNT], o->stacks[SlotID(0)]->count, false);
	//addProperty(QObject::tr("Resources reward"), o->resources); //TODO: implement in setProperty
}

void Inspector::updateProperties(CRewardableObject * o)
{
	if(!o) return;
		
	auto * delegate = new RewardsDelegate(*controller.map(), *o);
	addProperty(trReused[REWARD], PropertyEditorPlaceholder(), delegate, false);
}

void Inspector::updateProperties(CGPandoraBox * o)
{
	if(!o) return;
	
	addProperty(trReused[MESSAGE], o->message, new MessageDelegate, false);
}

void Inspector::updateProperties(CGEvent * o)
{
	if(!o) return;
	
	addProperty(trReused[REMOVE_AFTER], o->removeAfterVisit, false);
	addProperty(trReused[HUMAN_TRIGGER], o->humanActivate, false);
	addProperty(trReused[CPU_TRIGGER], o->computerActivate, false);
	//ui8 availableFor; //players whom this event is available for
}

void Inspector::updateProperties(CGSeerHut * o)
{
	if(!o || !o->quest) return;
	
	addProperty(trReused[FIRST_VISIT_TEXT], o->quest->firstVisitText, new MessageDelegate, false);
	addProperty(trReused[NEXT_VISIT_TEXT], o->quest->nextVisitText, new MessageDelegate, false);
	addProperty(trReused[COMPLETED_TEXT], o->quest->completedText, new MessageDelegate, false);
	addProperty(trReused[REPEAT_QUEST], o->quest->repeatedQuest, false);
	addProperty(trReused[TIME_LIMIT], o->quest->lastDay, false);
	
	{ //Quest
		auto * delegate = new QuestDelegate(controller, *o->quest);
		addProperty(QObject::tr("Quest"), PropertyEditorPlaceholder(), delegate, false);
	}
}

void Inspector::updateProperties(CGQuestGuard * o)
{
	if(!o || !o->quest) return;
	
	addProperty(trReused[REWARD], PropertyEditorPlaceholder(), nullptr, true);
	addProperty(trReused[REPEAT_QUEST], o->quest->repeatedQuest, true);
}

void Inspector::updateProperties()
{
	if(!obj)
		return;
	table->setRowCount(0); //cleanup table
	
	addProperty(QObject::tr("Identifier"), obj);
	addProperty(QObject::tr("ID"), obj->ID.getNum());
	addProperty(QObject::tr("SubID"), obj->subID);
	addProperty(QObject::tr("InstanceName"), obj->instanceName);
	
	if(obj->ID != Obj::HERO_PLACEHOLDER && !dynamic_cast<CGHeroInstance*>(obj))
	{
		auto factory = VLC->objtypeh->getHandlerFor(obj->ID, obj->subID);
		addProperty(QObject::tr("IsStatic"), factory->isStaticObject());
	}
	
	addProperty(trReused[OWNER], obj->tempOwner, new OwnerDelegate(controller), true);
	
	UPDATE_OBJ_PROPERTIES(CArmedInstance);
	UPDATE_OBJ_PROPERTIES(CGResource);
	UPDATE_OBJ_PROPERTIES(CGArtifact);
	UPDATE_OBJ_PROPERTIES(CGMine);
	UPDATE_OBJ_PROPERTIES(CGGarrison);
	UPDATE_OBJ_PROPERTIES(CGShipyard);
	UPDATE_OBJ_PROPERTIES(CGDwelling);
	UPDATE_OBJ_PROPERTIES(CGTownInstance);
	UPDATE_OBJ_PROPERTIES(CGCreature);
	UPDATE_OBJ_PROPERTIES(CGHeroPlaceholder);
	UPDATE_OBJ_PROPERTIES(CGHeroInstance);
	UPDATE_OBJ_PROPERTIES(CGSignBottle);
	UPDATE_OBJ_PROPERTIES(FlaggableMapObject);
	UPDATE_OBJ_PROPERTIES(CRewardableObject);
	UPDATE_OBJ_PROPERTIES(CGPandoraBox);
	UPDATE_OBJ_PROPERTIES(CGEvent);
	UPDATE_OBJ_PROPERTIES(CGSeerHut);
	UPDATE_OBJ_PROPERTIES(CGQuestGuard);
	
	table->show();
}

//===============IMPLEMENT PROPERTY UPDATE================================
void Inspector::setProperty(const QString & key, const QTableWidgetItem * item)
{
	if(!item->data(Qt::UserRole).isNull())
	{
		setProperty(key, item->data(Qt::UserRole));
		return;
	}
	
	if(item->flags() & Qt::ItemIsUserCheckable)
	{
		setProperty(key, QVariant::fromValue(item->checkState() == Qt::Checked));
		return;
	}
	
	setProperty(key, item->text());
}

void Inspector::setProperty(const QString & key, const QVariant & value)
{
	if(!obj)
		return;
	
	if(key == trReused[OWNER])
		obj->tempOwner = PlayerColor(value.toInt());
	
	SET_PROPERTIES(CArmedInstance);
	SET_PROPERTIES(CGTownInstance);
	SET_PROPERTIES(CGArtifact);
	SET_PROPERTIES(CGMine);
	SET_PROPERTIES(CGResource);
	SET_PROPERTIES(CGDwelling);
	SET_PROPERTIES(CGGarrison);
	SET_PROPERTIES(CGCreature);
	SET_PROPERTIES(CGHeroPlaceholder);
	SET_PROPERTIES(CGHeroInstance);
	SET_PROPERTIES(CGShipyard);
	SET_PROPERTIES(CGSignBottle);
	SET_PROPERTIES(FlaggableMapObject);
	SET_PROPERTIES(CRewardableObject);
	SET_PROPERTIES(CGPandoraBox);
	SET_PROPERTIES(CGEvent);
	SET_PROPERTIES(CGSeerHut);
	SET_PROPERTIES(CGQuestGuard);
}

void Inspector::setProperty(CArmedInstance * o, const QString & key, const QVariant & value)
{
	if(!o) return;
}

void Inspector::setProperty(FlaggableMapObject * o, const QString & key, const QVariant & value)
{
	if(!o) return;
}

void Inspector::setProperty(CRewardableObject * o, const QString & key, const QVariant & value)
{
	if(!o) return;
}

void Inspector::setProperty(CGPandoraBox * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == trReused[MESSAGE])
		o->message = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("guards", o->instanceName, "message"), value.toString().toStdString()));
}

void Inspector::setProperty(CGEvent * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == trReused[REMOVE_AFTER])
		o->removeAfterVisit = value.toBool();
	
	if(key == trReused[HUMAN_TRIGGER])
		o->humanActivate = value.toBool();
	
	if(key == trReused[CPU_TRIGGER])
		o->computerActivate = value.toBool();
}

void Inspector::setProperty(CGTownInstance * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == trReused[TOWN_NAME])
		o->setNameTextId(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("town", o->instanceName, "name"), value.toString().toStdString()));
}

void Inspector::setProperty(CGSignBottle * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == trReused[MESSAGE])
		o->message = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("sign", o->instanceName, "message"), value.toString().toStdString()));
}

void Inspector::setProperty(CGMine * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == trReused[PRODUCTIVITY])
		o->producedQuantity = value.toString().toInt();
}

void Inspector::setProperty(CGArtifact * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == trReused[MESSAGE])
		o->message = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("guards", o->instanceName, "message"), value.toString().toStdString()));
	
	if(o->storedArtifact && key == trReused[SPELL])
	{
		o->storedArtifact = ArtifactUtils::createScroll(SpellID(value.toInt()));
	}
}

void Inspector::setProperty(CGDwelling * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == trReused[SAME_AS_TOWN])
	{
		if (!o->randomizationInfo.has_value())
			o->randomizationInfo = CGDwellingRandomizationInfo();

		o->randomizationInfo->instanceId = "";
		if(CGTownInstance * town = data_cast<CGTownInstance>(value.toLongLong()))
			o->randomizationInfo->instanceId = town->instanceName;
	}
}

void Inspector::setProperty(CGGarrison * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == trReused[REMOVABLE_UNITS])
		o->removableUnits = value.toBool();
}

void Inspector::setProperty(CGHeroPlaceholder * o, const QString & key, const QVariant & value)
{
	if(!o) return;

	if(key == trReused[PLACEHOLDER_TYPE])
	{
		if(value.toBool())
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
		
		updateProperties();
	}
	
	if(key == trReused[POWER_RANK])
		o->powerRank = value.toInt();
	
	if(key == trReused[HERO_TYPE])
	{
		o->heroType = HeroTypeID(value.toInt());
	}
}

void Inspector::setProperty(CGHeroInstance * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == trReused[GENDER])
		o->gender = EHeroGender(value.toInt());
	
	if(key == trReused[NAME])
		o->nameCustomTextId = mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("hero", o->instanceName, "name"), value.toString().toStdString());
	
	if(key == trReused[BIOGRAPHY])
		o->biographyCustomTextId = mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("hero", o->instanceName, "biography"), value.toString().toStdString());
	
	if(key == trReused[EXPERIENCE])
		o->exp = value.toString().toInt();
	
	if(key == trReused[HERO_TYPE])
	{
		for(auto const & t : VLC->heroh->objects)
		{
			if(t->getId() == value.toInt())
				o->subID = value.toInt();
		}
		o->gender = o->getHeroType()->gender;
		o->randomizeArmy(o->getHeroType()->heroClass->faction);
		updateProperties(); //updating other properties after change
	}

	if(key == trReused[PATROL_RADIUS])
	{
		auto radius = value.toInt();
		o->patrol.patrolRadius = radius;
		o->patrol.patrolling = radius != CGHeroInstance::NO_PATROLLING;
	}
}

void Inspector::setProperty(CGShipyard * o, const QString & key, const QVariant & value)
{
	if(!o) return;
}

void Inspector::setProperty(CGResource * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == trReused[AMOUNT])
		o->amount = value.toString().toInt();
}

void Inspector::setProperty(CGCreature * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == trReused[MESSAGE])
		o->message = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("monster", o->instanceName, "message"), value.toString().toStdString()));
	if(key == trReused[CHARACTER])
		o->character = CGCreature::Character(value.toInt());
	if(key == trReused[NEVER_FLEES])
		o->neverFlees = value.toBool();
	if(key == trReused[NOT_GROWING])
		o->notGrowingTeam = value.toBool();
	if(key == trReused[AMOUNT])
		o->stacks[SlotID(0)]->count = value.toString().toInt();
}

void Inspector::setProperty(CGSeerHut * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == trReused[FIRST_VISIT_TEXT])
		o->quest->firstVisitText = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("quest", o->instanceName, "firstVisit"), value.toString().toStdString()));
	if(key == trReused[NEXT_VISIT_TEXT])
		o->quest->nextVisitText = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("quest", o->instanceName, "nextVisit"), value.toString().toStdString()));
	if(key == trReused[COMPLETED_TEXT])
		o->quest->completedText = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("quest", o->instanceName, "completed"), value.toString().toStdString()));
	if(key == trReused[REPEAT_QUEST])
		o->quest->repeatedQuest = value.toBool();
	if(key == trReused[TIME_LIMIT])
		o->quest->lastDay = value.toString().toInt();
}

void Inspector::setProperty(CGQuestGuard * o, const QString & key, const QVariant & value)
{
	if(!o) return;
}


//===============IMPLEMENT PROPERTY VALUE TYPE============================
QTableWidgetItem * Inspector::addProperty(CGObjectInstance * value)
{
	auto * item = new QTableWidgetItem(QString::number(data_cast<CGObjectInstance>(value)));
	item->setFlags(Qt::NoItemFlags);
	return item;
}

QTableWidgetItem * Inspector::addProperty(Inspector::PropertyEditorPlaceholder value)
{
	auto item = new QTableWidgetItem("...");
	item->setFlags(Qt::NoItemFlags);
	return item;
}

QTableWidgetItem * Inspector::addProperty(unsigned int value)
{
	auto * item = new QTableWidgetItem(QString::number(value));
	item->setFlags(Qt::NoItemFlags);
	//item->setData(Qt::UserRole, QVariant::fromValue(value));
	return item;
}

QTableWidgetItem * Inspector::addProperty(int value)
{
	auto * item = new QTableWidgetItem(QString::number(value));
	item->setFlags(Qt::NoItemFlags);
	//item->setData(Qt::UserRole, QVariant::fromValue(value));
	return item;
}

QTableWidgetItem * Inspector::addProperty(bool value)
{
	auto item = new QTableWidgetItem;
	item->setFlags(Qt::ItemIsUserCheckable);
	item->setCheckState(value ? Qt::Checked : Qt::Unchecked);
	return item;
}

QTableWidgetItem * Inspector::addProperty(const std::string & value)
{
	return addProperty(QString::fromStdString(value));
}

QTableWidgetItem * Inspector::addProperty(const TextIdentifier & value)
{
	return addProperty(VLC->generaltexth->translate(value.get()));
}

QTableWidgetItem * Inspector::addProperty(const MetaString & value)
{
	return addProperty(value.toString());
}

QTableWidgetItem * Inspector::addProperty(const QString & value)
{
	auto * item = new QTableWidgetItem(value);
	item->setFlags(Qt::NoItemFlags);
	return item;
}

QTableWidgetItem * Inspector::addProperty(const int3 & value)
{
	auto * item = new QTableWidgetItem(QString("(%1, %2, %3)").arg(value.x, value.y, value.z));
	item->setFlags(Qt::NoItemFlags);
	return item;
}

QTableWidgetItem * Inspector::addProperty(const PlayerColor & value)
{
	auto str = QObject::tr("UNFLAGGABLE");
	if(value == PlayerColor::NEUTRAL)
		str = QObject::tr("neutral");
	
	if(value.isValidPlayer())
		str = QString::fromStdString(GameConstants::PLAYER_COLOR_NAMES[value]);
	
	auto * item = new QTableWidgetItem(str);
	item->setFlags(Qt::NoItemFlags);
	item->setData(Qt::UserRole, QVariant::fromValue(value.getNum()));
	return item;
}

QTableWidgetItem * Inspector::addProperty(const GameResID & value)
{
	auto * item = new QTableWidgetItem(QString::fromStdString(GameConstants::RESOURCE_NAMES[value.toEnum()]));
	item->setFlags(Qt::NoItemFlags);
	item->setData(Qt::UserRole, QVariant::fromValue(value.getNum()));
	return item;
}

QTableWidgetItem * Inspector::addProperty(CGCreature::Character value)
{
	auto * item = new QTableWidgetItem;
	item->setFlags(Qt::NoItemFlags);
	item->setData(Qt::UserRole, QVariant::fromValue(int(value)));
	
	for(auto & i : CharacterIdentifiers)
	{
		if(i.second.toInt() == value)
		{
			item->setText(i.first);
			break;
		}
	}
	
	return item;
}

//========================================================================

Inspector::Inspector(MapController & c, CGObjectInstance * o, QTableWidget * t): obj(o), table(t), controller(c)
{
}

/*
 * Delegates
 */

QWidget * InspectorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return new QComboBox(parent);
}

void InspectorDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	if(QComboBox *ed = qobject_cast<QComboBox *>(editor))
	{
		for(auto & i : options)
		{
			ed->addItem(i.first);
			ed->setItemData(ed->count() - 1, i.second);
		}
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void InspectorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if(QComboBox *ed = qobject_cast<QComboBox *>(editor))
	{
		if(!options.isEmpty())
		{
			QMap<int, QVariant> data;
			data[Qt::DisplayRole] = options[ed->currentIndex()].first;
			data[Qt::UserRole] = options[ed->currentIndex()].second;
			model->setItemData(index, data);
		}
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

OwnerDelegate::OwnerDelegate(MapController & controller, bool addNeutral)
{
	if(addNeutral)
		options.push_back({QObject::tr("neutral"), QVariant::fromValue(PlayerColor::NEUTRAL.getNum()) });
	for(int p = 0; p < controller.map()->players.size(); ++p)
		if(controller.map()->players[p].canAnyonePlay())
			options.push_back({QString::fromStdString(GameConstants::PLAYER_COLOR_NAMES[p]), QVariant::fromValue(PlayerColor(p).getNum()) });
}
