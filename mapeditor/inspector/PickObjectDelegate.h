/*
 * PickObjectDelegate.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QItemDelegate>
#include "baseinspectoritemdelegate.h"

class MapController;

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;

VCMI_LIB_NAMESPACE_END

class PickObjectDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	PickObjectDelegate(MapController &);
	PickObjectDelegate(MapController &, std::function<bool(const CGObjectInstance *)>);
	
	template<class T>
	static bool typedFilter(const CGObjectInstance * o)
	{
		return dynamic_cast<const T*>(o) != nullptr;
	}

	QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
	
public slots:
	void onObjectPicked(const CGObjectInstance *);
	
private:
	MapController & controller;
	std::function<bool(const CGObjectInstance *)> filter;
	mutable QModelIndex modelIndex;
};
