/*
 * PickObjectDelegate.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "PickObjectDelegate.h"

#include "../mapcontroller.h"
#include "../../lib/mapObjects/CGObjectInstance.h"

PickObjectDelegate::PickObjectDelegate(MapController & c): controller(c)
{
	filter = [](const CGObjectInstance *)
	{
		return true;
	};
}

PickObjectDelegate::PickObjectDelegate(MapController & c, std::function<bool(const CGObjectInstance *)> f): controller(c), filter(f)
{
	
}

void PickObjectDelegate::onObjectPicked(const CGObjectInstance * o)
{
	for(MapScene * level : controller.getScenes())
	{
		auto & l = level->objectPickerView;
		l.clear();
		l.update();
		QObject::disconnect(&l, &ObjectPickerLayer::selectionMade, this, &PickObjectDelegate::onObjectPicked);
	}
	
	QMap<int, QVariant> data;
	data[Qt::DisplayRole] = QVariant("None");
	data[Qt::UserRole] = QVariant::fromValue(data_cast(o));
	if(o)
		data[Qt::DisplayRole] = QVariant(QString::fromStdString(o->instanceName));
	const_cast<QAbstractItemModel*>(modelIndex.model())->setItemData(modelIndex, data);
}

QWidget * PickObjectDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	for(MapScene * level : controller.getScenes())
	{
		auto & l = level->objectPickerView;
		l.highlight(filter);
		l.update();
		QObject::connect(&l, &ObjectPickerLayer::selectionMade, this, &PickObjectDelegate::onObjectPicked);
	}
	
	modelIndex = index;
	return nullptr;
}
