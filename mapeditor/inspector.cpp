#include "StdInc.h"
#include "inspector.h"
#include "../lib/CArtHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/mapObjects/CObjectClassesHandler.h"

//===============IMPLEMENT OBJECT INITIALIZATION FUNCTIONS================
Initializer::Initializer(CGObjectInstance * o)
{
///IMPORTANT! initialize order should be from base objects to derived objects
	INIT_OBJ_TYPE(CGResource);
	INIT_OBJ_TYPE(CGArtifact);
	INIT_OBJ_TYPE(CArmedInstance);
	INIT_OBJ_TYPE(CGMine);
	INIT_OBJ_TYPE(CGTownInstance);
}

void initialize(CArmedInstance * o)
{
	if(!o) return;
	
	o->tempOwner = PlayerColor::NEUTRAL;
}

void initialize(CGTownInstance * o)
{
	if(!o) return;

	o->builtBuildings.insert(BuildingID::FORT);
	o->builtBuildings.insert(BuildingID::DEFAULT);

	for(auto spell : VLC->spellh->objects) //add all regular spells to town
	{
		if(!spell->isSpecial() && !spell->isCreatureAbility())
			o->possibleSpells.push_back(spell->id);
	}
}

void initialize(CGArtifact * o)
{
	if(!o) return;
	
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
}

void initialize(CGMine * o)
{
	if(!o) return;
	
	o->producedResource = Res::ERes(o->subID);
	o->producedQuantity = o->defaultResProduction();
}

void initialize(CGResource * o)
{
	if(!o) return;
	
	o->amount = CGResource::RANDOM_AMOUNT;
}

//===============IMPLEMENT PROPERTIES SETUP===============================
void Inspector::updateProperties(CArmedInstance * o)
{
	if(!o) return;
	
	addProperty("Owner", o->tempOwner);
}

void Inspector::updateProperties(CGTownInstance * o)
{
	if(!o) return;
	
	addProperty("Owner", o->tempOwner, false);
	addProperty("Town name", o->name, false);
}

void Inspector::updateProperties(CGArtifact * o)
{
	if(!o) return;
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
	auto factory = VLC->objtypeh->getHandlerFor(obj->ID, obj->subID);
	addProperty("IsStatic", factory->isStaticObject());
	
	UPDATE_OBJ_PROPERTIES(CArmedInstance);
	UPDATE_OBJ_PROPERTIES(CGTownInstance);
	UPDATE_OBJ_PROPERTIES(CGArtifact);
	UPDATE_OBJ_PROPERTIES(CGMine);
	UPDATE_OBJ_PROPERTIES(CGResource);
	
	table->show();
}

//===============IMPLEMENT PROPERTY UPDATE================================
void Inspector::setProperty(const QString & key, const QVariant & value)
{
	if(!obj)
		return;
	
	SET_PROPERTIES(CArmedInstance);
	SET_PROPERTIES(CGTownInstance);
	SET_PROPERTIES(CGArtifact);
	SET_PROPERTIES(CGMine);
	SET_PROPERTIES(CGResource);
}

void Inspector::setProperty(CArmedInstance * object, const QString & key, const QVariant & value)
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

void Inspector::setProperty(CGTownInstance * object, const QString & key, const QVariant & value)
{
	if(!object)
		return;
	
	if(key == "Town name")
		object->name = value.toString().toStdString();
}

void Inspector::setProperty(CGMine * object, const QString & key, const QVariant & value)
{
	if(!object)
		return;
	
	if(key == "Productivity")
		object->producedQuantity = value.toString().toInt();
}

void Inspector::setProperty(CGArtifact * object, const QString & key, const QVariant & value)
{
	if(!object)
		return;
}

void Inspector::setProperty(CGResource * object, const QString & key, const QVariant & value)
{
	if(!object)
		return;
	
	if(key == "Amount")
		object->amount = value.toString().toInt();
}

//===============IMPLEMENT PROPERTY VALUE TYPE============================
QTableWidgetItem * Inspector::addProperty(CGObjectInstance * value)
{
	using NumericPointer = unsigned long long;
	static_assert(sizeof(CGObjectInstance *) == sizeof(NumericPointer),
				  "Compilied for 64 bit arcitecture. Use NumericPointer = unsigned int");
	return new QTableWidgetItem(QString::number(reinterpret_cast<NumericPointer>(value)));
}

QTableWidgetItem * Inspector::addProperty(unsigned int value)
{
	return new QTableWidgetItem(QString::number(value));
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

QTableWidgetItem * Inspector::addProperty(const Res::ERes & value)
{
	QString str;
	switch (value) {
		case Res::ERes::WOOD:
			str = "WOOD";
			break;
		case Res::ERes::ORE:
			str = "ORE";
			break;
		case Res::ERes::SULFUR:
			str = "SULFUR";
			break;
		case Res::ERes::GEMS:
			str = "GEMS";
			break;
		case Res::ERes::MERCURY:
			str = "MERCURY";
			break;
		case Res::ERes::CRYSTAL:
			str = "CRYSTAL";
			break;
		case Res::ERes::GOLD:
			str = "GOLD";
			break;
		default:
			break;
	}
	return new QTableWidgetItem(str);
}

//========================================================================

Inspector::Inspector(CGObjectInstance * o, QTableWidget * t): obj(o), table(t)
{
}

/*
 * Delegates
 */

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
