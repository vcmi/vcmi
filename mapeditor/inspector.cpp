#include "StdInc.h"
#include "inspector.h"
#include "../lib/mapObjects/CObjectHandler.h"
#include "../lib/mapObjects/CObjectClassesHandler.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/mapObjects/MiscObjects.h"
#include "../lib/CArtHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CRandomGenerator.h"

void Inspector::setProperty(const QString & key, const QVariant & value)
{
	if(!obj)
		return;

	setProperty(dynamic_cast<CGTownInstance*>(obj), key, value);
	//updateProperties();
}

void Inspector::setProperty(CGTownInstance * object, const QString & key, const QVariant & value)
{
	if(!object)
		return;

	if(key == "Owner")
	{
		PlayerColor owner(value.toString().toInt());
		if(value == "NEUTRAL")
			owner = PlayerColor::NEUTRAL;
		if(value == "UNFLAGGABLE")
			owner = PlayerColor::UNFLAGGABLE;
		object->tempOwner = owner;
		return;
	}
}

CGTownInstance * initialize(CGTownInstance * o)
{
	if(!o)
		return nullptr;

	o->tempOwner = PlayerColor::NEUTRAL;
	o->builtBuildings.insert(BuildingID::FORT);
	o->builtBuildings.insert(BuildingID::DEFAULT);

	for(auto spell : VLC->spellh->objects) //add all regular spells to town
	{
		if(!spell->isSpecial() && !spell->isCreatureAbility())
			o->possibleSpells.push_back(spell->id);
	}
	return o;
}

CGArtifact * initialize(CGArtifact * o)
{
	if(!o)
		return nullptr;
	
	if(o->ID == Obj::SPELL_SCROLL)
	{
		std::vector<SpellID> out;
		for(auto spell : VLC->spellh->objects) //spellh size appears to be greater (?)
		{
			if(/*map.isAllowedSpell(spell->id) && spell->level == i + 1*/ true)
			{
				out.push_back(spell->id);
			}
		}
		auto a = CArtifactInstance::createScroll(*RandomGeneratorUtil::nextItem(out, CRandomGenerator::getDefault()));
		o->storedArtifact = a;
	}
	
	return o;
}

Initializer::Initializer(CGObjectInstance * o)
{
	initialize(dynamic_cast<CGTownInstance*>(o));
	initialize(dynamic_cast<CGArtifact*>(o));
}

Inspector::Inspector(CGObjectInstance * o, QTableWidget * t): obj(o), table(t)
{
	/*
	/// Position of bottom-right corner of object on map
	int3 pos;
	/// Type of object, e.g. town, hero, creature.
	Obj ID;
	/// Subtype of object, depends on type
	si32 subID;
	/// Current owner of an object (when below PLAYER_LIMIT)
	PlayerColor tempOwner;
	/// Index of object in map's list of objects
	ObjectInstanceID id;
	/// Defines appearance of object on map (animation, blocked tiles, blit order, etc)
	ObjectTemplate appearance;
	/// If true hero can visit this object only from neighbouring tiles and can't stand on this object
	bool blockVisit;

	std::string instanceName;
	std::string typeName;
	std::string subTypeName;*/
}

void Inspector::updateProperties()
{
	if(!obj)
		return;

	addProperty("Indentifier", obj);
	addProperty("ID", obj->ID.getNum());
	addProperty("SubID", obj->subID);
	addProperty("InstanceName", obj->instanceName);
	addProperty("TypeName", obj->typeName);
	addProperty("SubTypeName", obj->subTypeName);
	addProperty("Owner", obj->tempOwner, false);

	auto factory = VLC->objtypeh->getHandlerFor(obj->ID, obj->subID);
	addProperty("IsStatic", factory->isStaticObject());

	table->show();
}

QTableWidgetItem * Inspector::addProperty(CGObjectInstance * value)
{
	using NumericPointer = unsigned long long;
	static_assert(sizeof(CGObjectInstance *) == sizeof(NumericPointer),
			"Compilied for 64 bit arcitecture. Use NumericPointer = unsigned int");
	return new QTableWidgetItem(QString::number(reinterpret_cast<NumericPointer>(value)));
}

QTableWidgetItem * Inspector::addProperty(int value)
{
	return new QTableWidgetItem(QString::number(value));
}

QTableWidgetItem * Inspector::addProperty(bool value)
{
	return new QTableWidgetItem(value ? "TRUE" : "FALSE");
}

QTableWidgetItem * Inspector::addProperty(const std::string & value)
{
	return addProperty(QString::fromStdString(value));
}

QTableWidgetItem * Inspector::addProperty(const QString & value)
{
	return new QTableWidgetItem(value);
}

QTableWidgetItem * Inspector::addProperty(const int3 & value)
{
	return new QTableWidgetItem(QString("(%1, %2, %3)").arg(value.x, value.y, value.z));
}

QTableWidgetItem * Inspector::addProperty(const PlayerColor & value)
{
	//auto str = QString("PLAYER %1").arg(value.getNum());
	auto str = QString::number(value.getNum());
	if(value == PlayerColor::NEUTRAL)
		str = "NEUTRAL";
	if(value == PlayerColor::UNFLAGGABLE)
		str = "UNFLAGGABLE";
	return new QTableWidgetItem(str);
}

QWidget * PlayerColorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if (index.data().canConvert<int>())
	{
		auto *editor = new QComboBox(parent);
		connect(editor, SIGNAL(activated(int)), this, SLOT(commitAndCloseEditor(int)));
		return editor;
	}
	return QStyledItemDelegate::createEditor(parent, option, index);
}

void PlayerColorDelegate::commitAndCloseEditor(int id)
{
	QComboBox *editor = qobject_cast<QComboBox *>(sender());
	emit commitData(editor);
	emit closeEditor(editor);
}

void PlayerColorDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	if (index.data().canConvert<int>())
	{
		PlayerColor player(qvariant_cast<int>(index.data()));
		QComboBox *ed = qobject_cast<QComboBox *>(editor);
		ed->addItem(QString::number(player.getNum()));
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void PlayerColorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if (index.data().canConvert<int>())
	{
		QComboBox *ed = qobject_cast<QComboBox *>(editor);
		model->setData(index, QVariant::fromValue(ed->currentText()));
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}
