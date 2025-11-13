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
#include "../lib/entities/hero/CHeroClass.h"
#include "../lib/entities/hero/CHeroHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../lib/mapObjectConstructors/CommonConstructors.h"
#include "../lib/mapObjects/ObjectTemplate.h"
#include "../lib/mapping/CMap.h"
#include "../lib/constants/StringConstants.h"
#include "../lib/CPlayerState.h"
#include "../lib/texts/CGeneralTextHandler.h"

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

//===============IMPLEMENT OBJECT INITIALIZATION FUNCTIONS================
Initializer::Initializer(MapController & controller, CGObjectInstance * o, const PlayerColor & pl)
	: controller(controller)
	, defaultPlayer(pl)
{
	logGlobal->info("New object instance initialized");
	o->cb = controller.getCallback();
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
		o->putStack(SlotID(0), std::make_unique<CStackInstance>(o->cb, CreatureID(o->subID), 1, false));
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
		for(auto const & t : LIBRARY->heroh->objects)
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
		for(auto const & spellId : LIBRARY->spellh->getDefaultAllowed()) //add all regular spells to town
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
		for(auto const & spell : LIBRARY->spellh->objects) //spellh size appears to be greater (?)
		{
			if(LIBRARY->spellh->getDefaultAllowed().count(spell->id) != 0)
			{
				out.push_back(spell->id);
			}
		}
		auto a = controller.map()->createScroll(*RandomGeneratorUtil::nextItem(out, CRandomGenerator::getDefault()));
		o->setArtifactInstance(a);
	}
	else if(o->ID == Obj::ARTIFACT || (o->ID >= Obj::RANDOM_ART && o->ID <= Obj::RANDOM_RELIC_ART))
	{
		auto instance = controller.map()->createArtifact(o->getArtifactType());
		o->setArtifactInstance(instance);
	}
	else
		throw std::runtime_error("Unimplemented initializer for CGArtifact object ID = "+ std::to_string(o->ID.getNum()));
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
		if(o->getResourceHandler()->getResourceType() == GameResID::NONE) // fallback
			o->producedResource = GameResID(o->subID);
		else
			o->producedResource = o->getResourceHandler()->getResourceType();
		o->producedQuantity = o->defaultResProduction();
	}
}

//===============IMPLEMENT PROPERTIES SETUP===============================
void Inspector::updateProperties(CArmedInstance * o)
{
	if(!o) return;
	
	auto * delegate = new ArmyDelegate(*o);
	addProperty(QObject::tr("Army"), PropertyEditorPlaceholder(), delegate, false);
}

void Inspector::updateProperties(CGDwelling * o)
{
	if(!o) return;
	
	addProperty(QObject::tr("Owner"), o->tempOwner, new OwnerDelegate(controller), false);
	
	if (o->ID == Obj::RANDOM_DWELLING || o->ID == Obj::RANDOM_DWELLING_LVL)
	{
		auto * delegate = new PickObjectDelegate(controller, PickObjectDelegate::typedFilter<CGTownInstance>);
		addProperty(QObject::tr("Same as town"), o->randomizationInfo, delegate, false);
	}
}

void Inspector::updateProperties(FlaggableMapObject * o)
{
	if(!o) return;

	addProperty(QObject::tr("Owner"), o->tempOwner, new OwnerDelegate(controller), false);
}

void Inspector::updateProperties(CGGarrison * o)
{
	if(!o) return;
	
	addProperty(QObject::tr("Owner"), o->tempOwner, new OwnerDelegate(controller), false);
	addProperty(QObject::tr("Removable units"), o->removableUnits, false);
}

void Inspector::updateProperties(CGShipyard * o)
{
	if(!o) return;
	
	addProperty(QObject::tr("Owner"), o->tempOwner, new OwnerDelegate(controller), false);
}

void Inspector::updateProperties(CGHeroPlaceholder * o)
{
	if(!o) return;
	
	addProperty(QObject::tr("Owner"), o->tempOwner, new OwnerDelegate(controller), false);
	
	bool type = false;
	if(o->heroType.has_value())
		type = true;
	else if(!o->powerRank.has_value())
		assert(0); //one of values must be initialized
	
	{
		auto * delegate = new InspectorDelegate;
		delegate->options = {{QObject::tr("POWER RANK"), QVariant::fromValue(false)}, {QObject::tr("HERO TYPE"), QVariant::fromValue(true)}};
		addProperty(QObject::tr("Placeholder type"), delegate->options[type].first, delegate, false);
	}
	
	addProperty(QObject::tr("Power rank"), o->powerRank.has_value() ? o->powerRank.value() : 0, type);
	
	{
		auto * delegate = new InspectorDelegate;
		for(int i = 0; i < LIBRARY->heroh->objects.size(); ++i)
		{
			delegate->options.push_back({QObject::tr(LIBRARY->heroh->objects[i]->getNameTranslated().c_str()), QVariant::fromValue(LIBRARY->heroh->objects[i]->getId().getNum())});
		}
		addProperty(QObject::tr("Hero type"), o->heroType.has_value() ? LIBRARY->heroh->getById(o->heroType.value())->getNameTranslated() : "", delegate, !type);
	}
}

void Inspector::updateProperties(CGHeroInstance * o)
{
	if(!o) return;
	
	auto isPrison = o->ID == Obj::PRISON;
	addProperty(QObject::tr("Owner"), o->tempOwner, new OwnerDelegate(controller, isPrison), isPrison); //field is not editable for prison
	addProperty<int>(QObject::tr("Experience"), o->exp, false);
	addProperty(QObject::tr("Hero class"), o->getHeroClassID().hasValue() ? o->getHeroClass()->getNameTranslated() : "", true);
	
	{ //Gender
		auto * delegate = new InspectorDelegate;
		delegate->options = {{QObject::tr("MALE"), QVariant::fromValue(static_cast<int>(EHeroGender::MALE))}, {QObject::tr("FEMALE"), QVariant::fromValue(static_cast<int>(EHeroGender::FEMALE))}};
		addProperty<std::string>(QObject::tr("Gender"), (o->gender == EHeroGender::FEMALE ? QObject::tr("FEMALE") : QObject::tr("MALE")).toStdString(), delegate , false);
	}
	addProperty(QObject::tr("Name"), o->getNameTranslated(), false);
	addProperty(QObject::tr("Biography"), o->getBiographyTranslated(), new MessageDelegate, false);
	addProperty(QObject::tr("Portrait"), PropertyEditorPlaceholder(), new PortraitDelegate(*o), false);
	
	auto * delegate = new HeroSkillsDelegate(*o);
	addProperty(QObject::tr("Skills"), PropertyEditorPlaceholder(), delegate, false);
	addProperty(QObject::tr("Spells"), PropertyEditorPlaceholder(), new HeroSpellDelegate(*o), false);
	addProperty(QObject::tr("Artifacts"), PropertyEditorPlaceholder(), new HeroArtifactsDelegate(controller, *o), false);
	
	if(o->getHeroTypeID().hasValue() || o->ID == Obj::PRISON)
	{ //Hero type
		auto * delegate = new InspectorDelegate;
		for(const auto & heroPtr : LIBRARY->heroh->objects)
		{
			if(controller.map()->allowedHeroes.count(heroPtr->getId()) != 0)
			{
				if(o->ID == Obj::PRISON || heroPtr->heroClass->getIndex() == o->getHeroClassID())
				{
					delegate->options.push_back({QObject::tr(heroPtr->getNameTranslated().c_str()), QVariant::fromValue(heroPtr->getIndex())});
				}
			}
		}
		addProperty(QObject::tr("Hero type"), o->getHeroTypeID().hasValue() ? o->getHeroType()->getNameTranslated() : "", delegate, false);
	}
	{
		const int maxRadius = 60;
		auto * patrolDelegate = new InspectorDelegate;
		patrolDelegate->options = { {QObject::tr("No patrol"), QVariant::fromValue(CGHeroInstance::NO_PATROLLING)} };
		for(int i = 0; i <= maxRadius; ++i)
			patrolDelegate->options.push_back({ QObject::tr("%n tile(s)", "", i), QVariant::fromValue(i)});
		auto patrolRadiusText = o->patrol.patrolling ? QObject::tr("%n tile(s)", "", o->patrol.patrolRadius) : QObject::tr("No patrol");
		addProperty(QObject::tr("Patrol radius"), patrolRadiusText, patrolDelegate, false);
	}
}

void Inspector::updateProperties(CGTownInstance * o)
{
	if(!o) return;
	
	addProperty(QObject::tr("Town name"), o->getNameTranslated(), false);
	
	auto * delegate = new TownBuildingsDelegate(*o);
	addProperty(QObject::tr("Buildings"), PropertyEditorPlaceholder(), delegate, false);
	addProperty(QObject::tr("Spells"), PropertyEditorPlaceholder(), new TownSpellsDelegate(*o), false);
	addProperty(QObject::tr("Events"), PropertyEditorPlaceholder(), new TownEventsDelegate(*o, controller), false);
	if(o->ID == Obj::RANDOM_TOWN)
		addProperty(QObject::tr("Same as player"), o->alignmentToPlayer, new OwnerDelegate(controller), false);
}

void Inspector::updateProperties(CGArtifact * o)
{
	if(!o) return;

	addProperty(QObject::tr("Message"), o->message, false);

	const CArtifactInstance * instance = o->getArtifactInstance();
	if(instance && o->ID == Obj::SPELL_SCROLL)
	{
		SpellID spellId = instance->getScrollSpellID();
		if(spellId != SpellID::NONE)
		{
			auto * delegate = new InspectorDelegate;
			for(auto const & spell : LIBRARY->spellh->objects)
			{
				if(controller.map()->allowedSpells.count(spell->id) != 0)
					delegate->options.push_back({QObject::tr(spell->getNameTranslated().c_str()), QVariant::fromValue(int(spell->getId()))});
			}
			addProperty(QObject::tr("Spell"), LIBRARY->spellh->getById(spellId)->getNameTranslated(), delegate, false);
		}
	}
}

void Inspector::updateProperties(CGMine * o)
{
	if(!o) return;
	
	addProperty(QObject::tr("Owner"), o->tempOwner, new OwnerDelegate(controller), false);
	if(o->producedResource != GameResID::NONE)
		addProperty(QObject::tr("Resource"), o->producedResource);
	addProperty(QObject::tr("Productivity"), o->producedQuantity);
}

void Inspector::updateProperties(CGResource * o)
{
	if(!o) return;
	
	addProperty(QObject::tr("Amount"), o->amount, false);
	addProperty(QObject::tr("Message"), o->message, false);
}

void Inspector::updateProperties(CGSignBottle * o)
{
	if(!o) return;
	
	addProperty(QObject::tr("Message"), o->message, new MessageDelegate, false);
}

void Inspector::updateProperties(CGCreature * o)
{
	if(!o) return;
	
	addProperty(QObject::tr("Message"), o->message, false);
	{ //Character
		auto * delegate = new InspectorDelegate;
		delegate->options = characterIdentifiers;
		addProperty<CGCreature::Character>("Character", (CGCreature::Character)o->character, delegate, false);
	}
	addProperty(QObject::tr("Never flees"), o->neverFlees, false);
	addProperty(QObject::tr("Not growing"), o->notGrowingTeam, false);
	addProperty(QObject::tr("Artifact reward"), o->gainedArtifact); //TODO: implement in setProperty
	addProperty(QObject::tr("Army"), PropertyEditorPlaceholder(), true);
	addProperty(QObject::tr("Amount"), o->stacks[SlotID(0)]->getCount(), false);
	//addProperty(QObject::tr("Resources reward"), o->resources); //TODO: implement in setProperty
}

void Inspector::updateProperties(CRewardableObject * o)
{
	if(!o) return;
		
	auto * delegate = new RewardsDelegate(*controller.map(), *o);
	addProperty(QObject::tr("Reward"), PropertyEditorPlaceholder(), delegate, false);
}

void Inspector::updateProperties(CGPandoraBox * o)
{
	if(!o) return;
	
	addProperty(QObject::tr("Message"), o->message, new MessageDelegate, false);
}

void Inspector::updateProperties(CGEvent * o)
{
	if(!o) return;
	
	addProperty(QObject::tr("Remove after"), o->removeAfterVisit, false);
	addProperty(QObject::tr("Human trigger"), o->humanActivate, false);
	addProperty(QObject::tr("Cpu trigger"), o->computerActivate, false);
	//ui8 availableFor; //players whom this event is available for
}

void Inspector::updateProperties(CGSeerHut * o)
{
	if(!o) return;
	
	addProperty(QObject::tr("First visit text"), o->getQuest().firstVisitText, new MessageDelegate, false);
	addProperty(QObject::tr("Next visit text"), o->getQuest().nextVisitText, new MessageDelegate, false);
	addProperty(QObject::tr("Completed text"), o->getQuest().completedText, new MessageDelegate, false);
	addProperty(QObject::tr("Repeat quest"), o->getQuest().repeatedQuest, false);
	addProperty(QObject::tr("Time limit"), o->getQuest().lastDay, false);
	
	{ //Quest
		auto * delegate = new QuestDelegate(controller, o->getQuest());
		addProperty(QObject::tr("Quest"), PropertyEditorPlaceholder(), delegate, false);
	}
}

void Inspector::updateProperties(CGQuestGuard * o)
{
	if(!o) return;
	
	addProperty(QObject::tr("Reward"), PropertyEditorPlaceholder(), nullptr, true);
	addProperty(QObject::tr("Repeat quest"), o->getQuest().repeatedQuest, true);
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
		auto factory = LIBRARY->objtypeh->getHandlerFor(obj->ID, obj->subID);
		addProperty(QObject::tr("IsStatic"), factory->isStaticObject());
	}
	
	addProperty(QObject::tr("Owner"), obj->tempOwner, new OwnerDelegate(controller), true);
	
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
	
	if(key == QObject::tr("Owner"))
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
	
	if(key == QObject::tr("Message"))
		o->message = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("guards", o->instanceName, "message"), value.toString().toStdString()));
}

void Inspector::setProperty(CGEvent * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == QObject::tr("Remove after"))
		o->removeAfterVisit = value.toBool();
	
	if(key == QObject::tr("Human trigger"))
		o->humanActivate = value.toBool();
	
	if(key == QObject::tr("Cpu trigger"))
		o->computerActivate = value.toBool();
}

void Inspector::setProperty(CGTownInstance * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == QObject::tr("Town name"))
		o->setNameTextId(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("town", o->instanceName, "name"), value.toString().toStdString()));

	if(key == QObject::tr("Same as player"))
		o->alignmentToPlayer = PlayerColor(value.toInt());
}

void Inspector::setProperty(CGSignBottle * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == QObject::tr("Message"))
		o->message = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("sign", o->instanceName, "message"), value.toString().toStdString()));
}

void Inspector::setProperty(CGMine * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == QObject::tr("Productivity"))
		o->producedQuantity = value.toString().toInt();
}

void Inspector::setProperty(CGArtifact * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == QObject::tr("Message"))
		o->message = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("guards", o->instanceName, "message"), value.toString().toStdString()));
	
	if(key == QObject::tr("Spell"))
	{
		o->setArtifactInstance(controller.map()->createScroll(SpellID(value.toInt())));
	}
}

void Inspector::setProperty(CGDwelling * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == QObject::tr("Same as town"))
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
	
	if(key == QObject::tr("Removable units"))
		o->removableUnits = value.toBool();
}

void Inspector::setProperty(CGHeroPlaceholder * o, const QString & key, const QVariant & value)
{
	if(!o) return;

	if(key == QObject::tr("Placeholder type"))
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
	
	if(key == QObject::tr("Power rank"))
		o->powerRank = value.toInt();
	
	if(key == QObject::tr("Hero type"))
	{
		o->heroType = HeroTypeID(value.toInt());
	}
}

void Inspector::setProperty(CGHeroInstance * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == QObject::tr("Gender"))
		o->gender = EHeroGender(value.toInt());
	
	if(key == QObject::tr("Name"))
		o->nameCustomTextId = mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("hero", o->instanceName, "name"), value.toString().toStdString());
	
	if(key == QObject::tr("Biography"))
		o->biographyCustomTextId = mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("hero", o->instanceName, "biography"), value.toString().toStdString());
	
	if(key == QObject::tr("Experience"))
		o->exp = value.toString().toInt();
	
	if(key == QObject::tr("Hero type"))
	{
		for(auto const & t : LIBRARY->heroh->objects)
		{
			if(t->getId() == value.toInt())
				o->subID = value.toInt();
		}
		o->gender = o->getHeroType()->gender;
		o->randomizeArmy(o->getHeroType()->heroClass->faction);
		updateProperties(); //updating other properties after change
	}

	if(key == QObject::tr("Patrol radius"))
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
	
	if(key == QObject::tr("Amount"))
		o->amount = value.toString().toInt();
}

void Inspector::setProperty(CGCreature * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == QObject::tr("Message"))
		o->message = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("monster", o->instanceName, "message"), value.toString().toStdString()));
	if(key == QObject::tr("Character"))
		o->character = CGCreature::Character(value.toInt());
	if(key == QObject::tr("Never flees"))
		o->neverFlees = value.toBool();
	if(key == QObject::tr("Not growing"))
		o->notGrowingTeam = value.toBool();
	if(key == QObject::tr("Amount"))
		o->stacks[SlotID(0)]->setCount(value.toString().toInt());
}

void Inspector::setProperty(CGSeerHut * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == QObject::tr("First visit text"))
		o->getQuest().firstVisitText = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("quest", o->instanceName, "firstVisit"), value.toString().toStdString()));
	if(key == QObject::tr("Next visit text"))
		o->getQuest().nextVisitText = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("quest", o->instanceName, "nextVisit"), value.toString().toStdString()));
	if(key == QObject::tr("Completed text"))
		o->getQuest().completedText = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller.map(),
			TextIdentifier("quest", o->instanceName, "completed"), value.toString().toStdString()));
	if(key == QObject::tr("Repeat quest"))
		o->getQuest().repeatedQuest = value.toBool();
	if(key == QObject::tr("Time limit"))
		o->getQuest().lastDay = value.toString().toInt();
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
	return addProperty(LIBRARY->generaltexth->translate(value.get()));
}

QTableWidgetItem * Inspector::addProperty(const MetaString & value)
{
	return addProperty(value.toString());
}

QTableWidgetItem * Inspector::addProperty(const QString & value)
{
	auto * item = new QTableWidgetItem(value);
	item->setFlags(Qt::NoItemFlags);
	item->setToolTip(value);
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
	
	MetaString playerStr;
	playerStr.appendName(value);
	if(value.isValidPlayer())
		str = QString::fromStdString(playerStr.toString());
	
	auto * item = new QTableWidgetItem(str);
	item->setFlags(Qt::NoItemFlags);
	item->setData(Qt::UserRole, QVariant::fromValue(value.getNum()));
	return item;
}

QTableWidgetItem * Inspector::addProperty(const GameResID & value)
{
	MetaString str;
	str.appendName(value);
	auto * item = new QTableWidgetItem(QString::fromStdString(str.toString()));
	item->setFlags(Qt::NoItemFlags);
	item->setData(Qt::UserRole, QVariant::fromValue(value.getNum()));
	return item;
}

QTableWidgetItem * Inspector::addProperty(CGCreature::Character value)
{
	auto * item = new QTableWidgetItem;
	item->setFlags(Qt::NoItemFlags);
	item->setData(Qt::UserRole, QVariant::fromValue(int(value)));
	
	for(const auto & i : characterIdentifiers)
	{
		if(i.second.toInt() == value)
		{
			item->setText(i.first);
			break;
		}
	}
	
	return item;
}

QTableWidgetItem * Inspector::addProperty(const std::optional<CGDwellingRandomizationInfo> & value)
{
	QString text = QObject::tr("Select town");
	if(value.has_value() && !value->instanceId.empty())
		text = QString::fromStdString(value->instanceId);

	auto * item = new QTableWidgetItem(text);
	item->setFlags(Qt::NoItemFlags);
	return item;
}

//========================================================================

Inspector::Inspector(MapController & c, CGObjectInstance * o, QTableWidget * t): obj(o), table(t), controller(c)
{
	characterIdentifiers = {
		{ QObject::tr("Compliant"), QVariant::fromValue(int(CGCreature::Character::COMPLIANT)) },
		{ QObject::tr("Friendly"), QVariant::fromValue(int(CGCreature::Character::FRIENDLY)) },
		{ QObject::tr("Aggressive"), QVariant::fromValue(int(CGCreature::Character::AGGRESSIVE)) },
		{ QObject::tr("Hostile"), QVariant::fromValue(int(CGCreature::Character::HOSTILE)) },
		{ QObject::tr("Savage"), QVariant::fromValue(int(CGCreature::Character::SAVAGE)) },
	};
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
		{
			MetaString str;
			str.appendName(PlayerColor(p));
			options.push_back({QString::fromStdString(str.toString()), QVariant::fromValue(PlayerColor(p).getNum()) });
		}
}
