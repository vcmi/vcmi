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
#include "../lib/int3.h"
#include "../lib/GameConstants.h"
#include "../lib/mapObjects/CGCreature.h"
#include "../lib/mapObjects/MapObjects.h"
#include "../lib/ResourceSet.h"

#define DECLARE_OBJ_TYPE(x) void initialize(x*);
#define DECLARE_OBJ_PROPERTY_METHODS(x) \
void updateProperties(x*); \
void setProperty(x*, const QString &, const QVariant &);

#define INIT_OBJ_TYPE(x) initialize(dynamic_cast<x*>(o))
#define UPDATE_OBJ_PROPERTIES(x) updateProperties(dynamic_cast<x*>(obj))
#define SET_PROPERTIES(x) setProperty(dynamic_cast<x*>(obj), key, value)


class Initializer
{
public:
	//===============DECLARE MAP OBJECTS======================================
	DECLARE_OBJ_TYPE(CArmedInstance);
	DECLARE_OBJ_TYPE(CGShipyard);
	DECLARE_OBJ_TYPE(CGTownInstance);
	DECLARE_OBJ_TYPE(CGArtifact);
	DECLARE_OBJ_TYPE(CGMine);
	DECLARE_OBJ_TYPE(CGResource);
	DECLARE_OBJ_TYPE(CGDwelling);
	DECLARE_OBJ_TYPE(CGGarrison);
	DECLARE_OBJ_TYPE(CGHeroInstance);
	DECLARE_OBJ_TYPE(CGCreature);
	DECLARE_OBJ_TYPE(CGSignBottle);
	DECLARE_OBJ_TYPE(CGLighthouse);
	//DECLARE_OBJ_TYPE(CGEvent);
	//DECLARE_OBJ_TYPE(CGPandoraBox);
	//DECLARE_OBJ_TYPE(CGSeerHut);
	
	Initializer(CGObjectInstance *, const PlayerColor &);

private:
	PlayerColor defaultPlayer;
};

class Inspector
{
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
	DECLARE_OBJ_PROPERTY_METHODS(CGHeroInstance);
	DECLARE_OBJ_PROPERTY_METHODS(CGCreature);
	DECLARE_OBJ_PROPERTY_METHODS(CGSignBottle);
	DECLARE_OBJ_PROPERTY_METHODS(CGLighthouse);
	DECLARE_OBJ_PROPERTY_METHODS(CGPandoraBox);
	DECLARE_OBJ_PROPERTY_METHODS(CGEvent);
	DECLARE_OBJ_PROPERTY_METHODS(CGSeerHut);

//===============DECLARE PROPERTY VALUE TYPE==============================
	QTableWidgetItem * addProperty(unsigned int value);
	QTableWidgetItem * addProperty(int value);
	QTableWidgetItem * addProperty(const std::string & value);
	QTableWidgetItem * addProperty(const QString & value);
	QTableWidgetItem * addProperty(const int3 & value);
	QTableWidgetItem * addProperty(const PlayerColor & value);
	QTableWidgetItem * addProperty(const GameResID & value);
	QTableWidgetItem * addProperty(bool value);
	QTableWidgetItem * addProperty(CGObjectInstance * value);
	QTableWidgetItem * addProperty(CGCreature::Character value);
	QTableWidgetItem * addProperty(CQuest::Emission value);
	QTableWidgetItem * addProperty(PropertyEditorPlaceholder value);
	
//===============END OF DECLARATION=======================================
	
public:
	Inspector(CMap *, CGObjectInstance *, QTableWidget *);

	void setProperty(const QString & key, const QVariant & value);

	void updateProperties();
	
protected:

	template<class T>
	void addProperty(const QString & key, const T & value, QAbstractItemDelegate * delegate, bool restricted)
	{
		auto * itemValue = addProperty(value);
		if(restricted)
			itemValue->setFlags(Qt::NoItemFlags);
		
		QTableWidgetItem * itemKey = nullptr;
		if(keyItems.contains(key))
		{
			itemKey = keyItems[key];
			table->setItem(table->row(itemKey), 1, itemValue);
			if(delegate)
				table->setItemDelegateForRow(table->row(itemKey), delegate);
		}
		else
		{
			itemKey = new QTableWidgetItem(key);
			itemKey->setFlags(Qt::NoItemFlags);
			keyItems[key] = itemKey;
			
			table->setRowCount(row + 1);
			table->setItem(row, 0, itemKey);
			table->setItem(row, 1, itemValue);
			table->setItemDelegateForRow(row, delegate);
			++row;
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
	CMap * map;
};




class InspectorDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	static InspectorDelegate * boolDelegate();
	
	using QStyledItemDelegate::QStyledItemDelegate;

	QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	
	QStringList options;
};

