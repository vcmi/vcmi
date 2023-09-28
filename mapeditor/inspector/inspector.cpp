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
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../lib/mapObjects/ObjectTemplate.h"
#include "../lib/mapping/CMap.h"
#include "../lib/constants/StringConstants.h"

#include "townbulidingswidget.h"
#include "armywidget.h"
#include "messagewidget.h"
#include "rewardswidget.h"
#include "questwidget.h"
#include "heroskillswidget.h"

static QList<std::pair<QString, QVariant>> MissionIdentifiers
{
	{QObject::tr("None"), QVariant::fromValue(int(CQuest::Emission::MISSION_NONE))},
	{QObject::tr("Reach level"), QVariant::fromValue(int(CQuest::Emission::MISSION_LEVEL))},
	{QObject::tr("Stats"), QVariant::fromValue(int(CQuest::Emission::MISSION_PRIMARY_STAT))},
	{QObject::tr("Kill hero"), QVariant::fromValue(int(CQuest::Emission::MISSION_KILL_HERO))},
	{QObject::tr("Kill monster"), QVariant::fromValue(int(CQuest::Emission::MISSION_KILL_CREATURE))},
	{QObject::tr("Artifact"), QVariant::fromValue(int(CQuest::Emission::MISSION_ART))},
	{QObject::tr("Army"), QVariant::fromValue(int(CQuest::Emission::MISSION_ARMY))},
	{QObject::tr("Resources"), QVariant::fromValue(int(CQuest::Emission::MISSION_RESOURCES))},
	{QObject::tr("Hero"), QVariant::fromValue(int(CQuest::Emission::MISSION_HERO))},
	{QObject::tr("Player"), QVariant::fromValue(int(CQuest::Emission::MISSION_PLAYER))},
};

static QList<std::pair<QString, QVariant>> CharacterIdentifiers
{
	{QObject::tr("Compliant"), QVariant::fromValue(int(CGCreature::Character::COMPLIANT))},
	{QObject::tr("Friendly"), QVariant::fromValue(int(CGCreature::Character::FRIENDLY))},
	{QObject::tr("Agressive"), QVariant::fromValue(int(CGCreature::Character::AGRESSIVE))},
	{QObject::tr("Hostile"), QVariant::fromValue(int(CGCreature::Character::HOSTILE))},
	{QObject::tr("Savage"), QVariant::fromValue(int(CGCreature::Character::SAVAGE))},
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
	INIT_OBJ_TYPE(CGHeroInstance);
	INIT_OBJ_TYPE(CGSignBottle);
	INIT_OBJ_TYPE(CGLighthouse);
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
	
	switch(o->ID)
	{
		case Obj::RANDOM_DWELLING:
		case Obj::RANDOM_DWELLING_LVL:
		case Obj::RANDOM_DWELLING_FACTION:
			o->initRandomObjectInfo();
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

void Initializer::initialize(CGLighthouse * o)
{
	if(!o) return;
	
	o->tempOwner = defaultPlayer;
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
		for(auto t : VLC->heroh->objects)
		{
			if(t->heroClass->getId() == HeroClassID(o->subID))
			{
				o->type = t;
				break;
			}
		}
	}
	
	if(o->type)
	{
		//	o->type = VLC->heroh->objects.at(o->subID);
		
		o->gender = o->type->gender;
		o->portrait = o->type->imageIndex;
		o->randomizeArmy(o->type->heroClass->faction);
	}
}

void Initializer::initialize(CGTownInstance * o)
{
	if(!o) return;

	const std::vector<std::string> castleLevels{"village", "fort", "citadel", "castle", "capitol"};
	int lvl = vstd::find_pos(castleLevels, o->appearance->stringID);
	o->builtBuildings.insert(BuildingID::DEFAULT);
	if(lvl > -1) o->builtBuildings.insert(BuildingID::TAVERN);
	if(lvl > 0) o->builtBuildings.insert(BuildingID::FORT);
	if(lvl > 1) o->builtBuildings.insert(BuildingID::CITADEL);
	if(lvl > 2) o->builtBuildings.insert(BuildingID::CASTLE);
	if(lvl > 3) o->builtBuildings.insert(BuildingID::CAPITOL);

	for(auto spell : VLC->spellh->objects) //add all regular spells to town
	{
		if(!spell->isSpecial() && !spell->isCreatureAbility())
			o->possibleSpells.push_back(spell->id);
	}
}

void Initializer::initialize(CGArtifact * o)
{
	if(!o) return;
	
	if(o->ID == Obj::SPELL_SCROLL)
	{
		std::vector<SpellID> out;
		for(auto spell : VLC->spellh->objects) //spellh size appears to be greater (?)
		{
			if(VLC->spellh->getDefaultAllowed().at(spell->id))
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
	addProperty("Army", PropertyEditorPlaceholder(), delegate, false);
}

void Inspector::updateProperties(CGDwelling * o)
{
	if(!o) return;
	
	addProperty("Owner", o->tempOwner, false);
}

void Inspector::updateProperties(CGLighthouse * o)
{
	if(!o) return;
	
	addProperty("Owner", o->tempOwner, false);
}

void Inspector::updateProperties(CGGarrison * o)
{
	if(!o) return;
	
	addProperty("Owner", o->tempOwner, false);
	addProperty("Removable units", o->removableUnits, false);
}

void Inspector::updateProperties(CGShipyard * o)
{
	if(!o) return;
	
	addProperty("Owner", o->tempOwner, false);
}

void Inspector::updateProperties(CGHeroInstance * o)
{
	if(!o) return;
	
	addProperty("Owner", o->tempOwner, o->ID == Obj::PRISON); //field is not editable for prison
	addProperty<int>("Experience", o->exp, false);
	addProperty("Hero class", o->type ? o->type->heroClass->getNameTranslated() : "", true);
	
	{ //Gender
		auto * delegate = new InspectorDelegate;
		delegate->options = {{"MALE", QVariant::fromValue(int(EHeroGender::MALE))}, {"FEMALE", QVariant::fromValue(int(EHeroGender::FEMALE))}};
		addProperty<std::string>("Gender", (o->gender == EHeroGender::FEMALE ? "FEMALE" : "MALE"), delegate , false);
	}
	addProperty("Name", o->nameCustomTextId, false);
	addProperty("Biography", o->biographyCustomTextId, new MessageDelegate, false);
	addProperty("Portrait", o->portrait, false);
	
	auto * delegate = new HeroSkillsDelegate(*o);
	addProperty("Skills", PropertyEditorPlaceholder(), delegate, false);
	
	if(o->type)
	{ //Hero type
		auto * delegate = new InspectorDelegate;
		for(int i = 0; i < VLC->heroh->objects.size(); ++i)
		{
			if(map->allowedHeroes.at(i))
			{
				if(o->ID == Obj::PRISON || (o->type && VLC->heroh->objects[i]->heroClass->getIndex() == o->type->heroClass->getIndex()))
				{
					delegate->options.push_back({QObject::tr(VLC->heroh->objects[i]->getNameTranslated().c_str()), QVariant::fromValue(VLC->heroh->objects[i]->getId().getNum())});
				}
			}
		}
		addProperty("Hero type", o->type->getNameTranslated(), delegate, false);
	}
}

void Inspector::updateProperties(CGTownInstance * o)
{
	if(!o) return;
	
	addProperty("Town name", o->getNameTranslated(), false);
	
	auto * delegate = new TownBuildingsDelegate(*o);
	addProperty("Buildings", PropertyEditorPlaceholder(), delegate, false);
}

void Inspector::updateProperties(CGArtifact * o)
{
	if(!o) return;
	
	addProperty("Message", o->message, false);
	
	CArtifactInstance * instance = o->storedArtifact;
	if(instance)
	{
		SpellID spellId = instance->getScrollSpellID();
		if(spellId != SpellID::NONE)
		{
			auto * delegate = new InspectorDelegate;
			for(auto spell : VLC->spellh->objects)
			{
				if(map->allowedSpells.at(spell->id))
					delegate->options.push_back({QObject::tr(spell->getNameTranslated().c_str()), QVariant::fromValue(int(spell->getId()))});
			}
			addProperty("Spell", VLC->spellh->getById(spellId)->getNameTranslated(), delegate, false);
		}
	}
}

void Inspector::updateProperties(CGMine * o)
{
	if(!o) return;
	
	addProperty("Owner", o->tempOwner, false);
	addProperty("Resource", o->producedResource);
	addProperty("Productivity", o->producedQuantity, false);
}

void Inspector::updateProperties(CGResource * o)
{
	if(!o) return;
	
	addProperty("Amount", o->amount, false);
	addProperty("Message", o->message, false);
}

void Inspector::updateProperties(CGSignBottle * o)
{
	if(!o) return;
	
	addProperty("Message", o->message, new MessageDelegate, false);
}

void Inspector::updateProperties(CGCreature * o)
{
	if(!o) return;
	
	addProperty("Message", o->message, false);
	{ //Character
		auto * delegate = new InspectorDelegate;
		delegate->options = CharacterIdentifiers;
		addProperty<CGCreature::Character>("Character", (CGCreature::Character)o->character, delegate, false);
	}
	addProperty("Never flees", o->neverFlees, false);
	addProperty("Not growing", o->notGrowingTeam, false);
	addProperty("Artifact reward", o->gainedArtifact); //TODO: implement in setProperty
	addProperty("Army", PropertyEditorPlaceholder(), true);
	addProperty("Amount", o->stacks[SlotID(0)]->count, false);
	//addProperty("Resources reward", o->resources); //TODO: implement in setProperty
}

void Inspector::updateProperties(CRewardableObject * o)
{
	if(!o) return;
		
	auto * delegate = new RewardsDelegate(*map, *o);
	addProperty("Reward", PropertyEditorPlaceholder(), delegate, false);
}

void Inspector::updateProperties(CGPandoraBox * o)
{
	if(!o) return;
	
	addProperty("Message", o->message, new MessageDelegate, false);
}

void Inspector::updateProperties(CGEvent * o)
{
	if(!o) return;
	
	addProperty("Remove after", o->removeAfterVisit, false);
	addProperty("Human trigger", o->humanActivate, false);
	addProperty("Cpu trigger", o->computerActivate, false);
	//ui8 availableFor; //players whom this event is available for
}

void Inspector::updateProperties(CGSeerHut * o)
{
	if(!o) return;

	{ //Mission type
		auto * delegate = new InspectorDelegate;
		delegate->options = MissionIdentifiers;
		addProperty<CQuest::Emission>("Mission type", o->quest->missionType, delegate, false);
	}
	
	addProperty("First visit text", o->quest->firstVisitText, new MessageDelegate, false);
	addProperty("Next visit text", o->quest->nextVisitText, new MessageDelegate, false);
	addProperty("Completed text", o->quest->completedText, new MessageDelegate, false);
	
	{ //Quest
		auto * delegate = new QuestDelegate(*map, *o);
		addProperty("Quest", PropertyEditorPlaceholder(), delegate, false);
	}
}

void Inspector::updateProperties()
{
	if(!obj)
		return;
	table->setRowCount(0); //cleanup table
	
	addProperty("Indentifier", obj);
	addProperty("ID", obj->ID.getNum());
	addProperty("SubID", obj->subID);
	addProperty("InstanceName", obj->instanceName);
	addProperty("TypeName", obj->typeName);
	addProperty("SubTypeName", obj->subTypeName);
	
	if(!dynamic_cast<CGHeroInstance*>(obj))
	{
		auto factory = VLC->objtypeh->getHandlerFor(obj->ID, obj->subID);
		addProperty("IsStatic", factory->isStaticObject());
	}
	
	auto * delegate = new InspectorDelegate();
	delegate->options.push_back({QObject::tr("neutral"), QVariant::fromValue(PlayerColor::NEUTRAL.getNum())});
	for(int p = 0; p < map->players.size(); ++p)
		if(map->players[p].canAnyonePlay())
			delegate->options.push_back({QString::fromStdString(GameConstants::PLAYER_COLOR_NAMES[p]), QVariant::fromValue(PlayerColor(p).getNum())});
	addProperty("Owner", obj->tempOwner, delegate, true);
	
	UPDATE_OBJ_PROPERTIES(CArmedInstance);
	UPDATE_OBJ_PROPERTIES(CGResource);
	UPDATE_OBJ_PROPERTIES(CGArtifact);
	UPDATE_OBJ_PROPERTIES(CGMine);
	UPDATE_OBJ_PROPERTIES(CGGarrison);
	UPDATE_OBJ_PROPERTIES(CGShipyard);
	UPDATE_OBJ_PROPERTIES(CGDwelling);
	UPDATE_OBJ_PROPERTIES(CGTownInstance);
	UPDATE_OBJ_PROPERTIES(CGCreature);
	UPDATE_OBJ_PROPERTIES(CGHeroInstance);
	UPDATE_OBJ_PROPERTIES(CGSignBottle);
	UPDATE_OBJ_PROPERTIES(CGLighthouse);
	UPDATE_OBJ_PROPERTIES(CRewardableObject);
	UPDATE_OBJ_PROPERTIES(CGPandoraBox);
	UPDATE_OBJ_PROPERTIES(CGEvent);
	UPDATE_OBJ_PROPERTIES(CGSeerHut);
	
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
	
	if(key == "Owner")
		obj->tempOwner = PlayerColor(value.toInt());
	
	SET_PROPERTIES(CArmedInstance);
	SET_PROPERTIES(CGTownInstance);
	SET_PROPERTIES(CGArtifact);
	SET_PROPERTIES(CGMine);
	SET_PROPERTIES(CGResource);
	SET_PROPERTIES(CGDwelling);
	SET_PROPERTIES(CGGarrison);
	SET_PROPERTIES(CGCreature);
	SET_PROPERTIES(CGHeroInstance);
	SET_PROPERTIES(CGShipyard);
	SET_PROPERTIES(CGSignBottle);
	SET_PROPERTIES(CGLighthouse);
	SET_PROPERTIES(CRewardableObject);
	SET_PROPERTIES(CGPandoraBox);
	SET_PROPERTIES(CGEvent);
	SET_PROPERTIES(CGSeerHut);
}

void Inspector::setProperty(CArmedInstance * o, const QString & key, const QVariant & value)
{
	if(!o) return;
}

void Inspector::setProperty(CGLighthouse * o, const QString & key, const QVariant & value)
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
	
	if(key == "Message")
		o->message.appendRawString(value.toString().toStdString());
}

void Inspector::setProperty(CGEvent * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == "Remove after")
		o->removeAfterVisit = value.toBool();
	
	if(key == "Human trigger")
		o->humanActivate = value.toBool();
	
	if(key == "Cpu trigger")
		o->computerActivate = value.toBool();
}

void Inspector::setProperty(CGTownInstance * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == "Town name")
		o->setNameTextId(value.toString().toStdString());
}

void Inspector::setProperty(CGSignBottle * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == "Message")
		o->message.appendRawString(value.toString().toStdString());
}

void Inspector::setProperty(CGMine * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == "Productivity")
		o->producedQuantity = value.toString().toInt();
}

void Inspector::setProperty(CGArtifact * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == "Message")
		o->message.appendRawString(value.toString().toStdString());
	
	if(o->storedArtifact && key == "Spell")
	{
		o->storedArtifact = ArtifactUtils::createScroll(SpellID(value.toInt()));
	}
}

void Inspector::setProperty(CGDwelling * o, const QString & key, const QVariant & value)
{
	if(!o) return;
}

void Inspector::setProperty(CGGarrison * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == "Removable units")
		o->removableUnits = value.toBool();
}

void Inspector::setProperty(CGHeroInstance * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == "Gender")
		o->gender = EHeroGender(value.toInt());
	
	if(key == "Name")
		o->nameCustomTextId = value.toString().toStdString();
	
	if(key == "Experience")
		o->exp = value.toString().toInt();
	
	if(key == "Hero type")
	{
		for(auto t : VLC->heroh->objects)
		{
			if(t->getNameTranslated() == value.toString().toStdString())
				o->type = t.get();
		}
		o->gender = o->type->gender;
		o->portrait = o->type->imageIndex;
		o->randomizeArmy(o->type->heroClass->faction);
		updateProperties(); //updating other properties after change
	}
}

void Inspector::setProperty(CGShipyard * o, const QString & key, const QVariant & value)
{
	if(!o) return;
}

void Inspector::setProperty(CGResource * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == "Amount")
		o->amount = value.toString().toInt();
}

void Inspector::setProperty(CGCreature * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == "Message")
		o->message.appendRawString(value.toString().toStdString());
	if(key == "Character")
		o->character = CGCreature::Character(value.toInt());
	if(key == "Never flees")
		o->neverFlees = value.toBool();
	if(key == "Not growing")
		o->notGrowingTeam = value.toBool();
	if(key == "Amount")
		o->stacks[SlotID(0)]->count = value.toString().toInt();
}

void Inspector::setProperty(CGSeerHut * o, const QString & key, const QVariant & value)
{
	if(!o) return;
	
	if(key == "Mission type")
		o->quest->missionType = CQuest::Emission(value.toInt());
	if(key == "First visit text")
		o->quest->firstVisitText.appendRawString(value.toString().toStdString());
	if(key == "Next visit text")
		o->quest->nextVisitText.appendRawString(value.toString().toStdString());
	if(key == "Completed text")
		o->quest->completedText.appendRawString(value.toString().toStdString());
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

QTableWidgetItem * Inspector::addProperty(CQuest::Emission value)
{
	auto * item = new QTableWidgetItem;
	item->setFlags(Qt::NoItemFlags);
	item->setData(Qt::UserRole, QVariant::fromValue(int(value)));
	
	for(auto & i : MissionIdentifiers)
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

Inspector::Inspector(CMap * m, CGObjectInstance * o, QTableWidget * t): obj(o), table(t), map(m)
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

