/*
 * inspector.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../StdInc.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStyledItemDelegate>
#include "baseinspectoritemdelegate.h"
#include "../lib/int3.h"
#include "../lib/GameConstants.h"
#include "../lib/mapObjects/CGCreature.h"
#include "../lib/mapObjects/CGResource.h"
#include "../lib/mapObjects/CQuest.h"
#include "../lib/mapObjects/MapObjects.h"
#include "../lib/mapObjects/FlaggableMapObject.h"
#include "../lib/mapObjects/CRewardableObject.h"
#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/ResourceSet.h"

#define DECLARE_OBJ_TYPE(x) void initialize(x*);
#define DECLARE_OBJ_PROPERTY_METHODS(x) \
void updateProperties(x*); \
void setProperty(x*, const QString &, const QVariant &);

#define INIT_OBJ_TYPE(x) initialize(dynamic_cast<x*>(o))
#define UPDATE_OBJ_PROPERTIES(x) updateProperties(dynamic_cast<x*>(obj))
#define SET_PROPERTIES(x) setProperty(dynamic_cast<x*>(obj), key, value)


class MapController;
class Initializer
{
public:
	//===============DECLARE MAP OBJECTS======================================
	DECLARE_OBJ_TYPE(CArmedInstance);
	DECLARE_OBJ_TYPE(CGShipyard);
	DECLARE_OBJ_TYPE(CGTownInstance);
	DECLARE_OBJ_TYPE(CGArtifact);
	DECLARE_OBJ_TYPE(CGMine);
	DECLARE_OBJ_TYPE(CGDwelling);
	DECLARE_OBJ_TYPE(CGGarrison);
	DECLARE_OBJ_TYPE(CGHeroPlaceholder);
	DECLARE_OBJ_TYPE(CGHeroInstance);
	DECLARE_OBJ_TYPE(CGCreature);
	DECLARE_OBJ_TYPE(CGSignBottle);
	DECLARE_OBJ_TYPE(FlaggableMapObject);
	//DECLARE_OBJ_TYPE(CRewardableObject);
	//DECLARE_OBJ_TYPE(CGEvent);
	//DECLARE_OBJ_TYPE(CGPandoraBox);
	//DECLARE_OBJ_TYPE(CGSeerHut);
	
	Initializer(MapController & controller, CGObjectInstance *, const PlayerColor &);

private:
	MapController & controller;
	PlayerColor defaultPlayer;
};

class Inspector
{
	QList<std::pair<QString, QVariant>> characterIdentifiers;

protected:
	struct PropertyEditorPlaceholder {};
	
//===============DECLARE PROPERTIES SETUP=================================
	DECLARE_OBJ_PROPERTY_METHODS(CArmedInstance);
	DECLARE_OBJ_PROPERTY_METHODS(CGTownInstance);
	DECLARE_OBJ_PROPERTY_METHODS(CGShipyard);
	DECLARE_OBJ_PROPERTY_METHODS(CGArtifact);
	DECLARE_OBJ_PROPERTY_METHODS(CGMine);
	DECLARE_OBJ_PROPERTY_METHODS(CGResource);
	DECLARE_OBJ_PROPERTY_METHODS(CGDwelling);
	DECLARE_OBJ_PROPERTY_METHODS(CGGarrison);
	DECLARE_OBJ_PROPERTY_METHODS(CGHeroPlaceholder);
	DECLARE_OBJ_PROPERTY_METHODS(CGHeroInstance);
	DECLARE_OBJ_PROPERTY_METHODS(CGCreature);
	DECLARE_OBJ_PROPERTY_METHODS(CGSignBottle);
	DECLARE_OBJ_PROPERTY_METHODS(FlaggableMapObject);
	DECLARE_OBJ_PROPERTY_METHODS(CRewardableObject);
	DECLARE_OBJ_PROPERTY_METHODS(CGPandoraBox);
	DECLARE_OBJ_PROPERTY_METHODS(CGEvent);
	DECLARE_OBJ_PROPERTY_METHODS(CGSeerHut);
	DECLARE_OBJ_PROPERTY_METHODS(CGQuestGuard);

//===============DECLARE PROPERTY VALUE TYPE==============================
	QTableWidgetItem * addProperty(unsigned int value);
	QTableWidgetItem * addProperty(int value);
	QTableWidgetItem * addProperty(const MetaString & value);
	QTableWidgetItem * addProperty(const TextIdentifier & value);
	QTableWidgetItem * addProperty(const std::string & value);
	QTableWidgetItem * addProperty(const QString & value);
	QTableWidgetItem * addProperty(const int3 & value);
	QTableWidgetItem * addProperty(const PlayerColor & value);
	QTableWidgetItem * addProperty(const GameResID & value);
	QTableWidgetItem * addProperty(bool value);
	QTableWidgetItem * addProperty(CGObjectInstance * value);
	QTableWidgetItem * addProperty(CGCreature::Character value);
	QTableWidgetItem * addProperty(const std::optional<CGDwellingRandomizationInfo> & value);
	QTableWidgetItem * addProperty(PropertyEditorPlaceholder value);
	
//===============END OF DECLARATION=======================================
	
public:
	Inspector(MapController &, CGObjectInstance *, QTableWidget *);

	void setProperty(const QString & key, const QTableWidgetItem * item);
	
	void setProperty(const QString & key, const QVariant & value);

	void updateProperties();
	
protected:

	template<class T>
	void addProperty(const QString & key, const T & value, BaseInspectorItemDelegate * delegate, bool restricted)
	{
		auto * itemValue = addProperty(value);
		if(!restricted)
			itemValue->setFlags(itemValue->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		if(!(itemValue->flags() & Qt::ItemIsUserCheckable))
			itemValue->setFlags(itemValue->flags() | Qt::ItemIsEditable);
		
		QTableWidgetItem * itemKey = nullptr;
		if(keyItems.contains(key))
		{
			itemKey = keyItems[key];
			table->setItem(table->row(itemKey), 1, itemValue);
			table->setItemDelegateForRow(table->row(itemKey), delegate);
		}
		else
		{
			itemKey = new QTableWidgetItem(key);
			keyItems[key] = itemKey;
			
			table->setRowCount(row + 1);
			table->setItem(row, 0, itemKey);
			table->setItem(row, 1, itemValue);
			table->setItemDelegateForRow(row, delegate);
			++row;
		}
		itemKey->setFlags(restricted ? Qt::NoItemFlags : Qt::ItemIsEnabled);
		if(delegate != nullptr)
		{
			QModelIndex index = table->model()->index(itemValue->row(), itemValue->column());
			delegate->updateModelData(table->model(), index);
		}
	}
	
	template<class T>
	void addProperty(const QString & key, const T & value, bool restricted = true)
	{
		addProperty<T>(key, value, nullptr, restricted);
	}
	
protected:
	int row = 0;
	QTableWidget * table;
	CGObjectInstance * obj;
	QMap<QString, QTableWidgetItem*> keyItems;
	MapController & controller;
};


class InspectorDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;

	QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	
	QList<std::pair<QString, QVariant>> options;
};


class OwnerDelegate : public InspectorDelegate
{
	Q_OBJECT
public:
	OwnerDelegate(MapController & controller, bool addNeutral = true);
};
