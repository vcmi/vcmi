/*
 * towneventswidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../StdInc.h"
#include <QDialog>
#include "baseinspectoritemdelegate.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../mapcontroller.h"

namespace Ui {
	class TownEventsWidget;
}

class TownEventsWidget : public QDialog
{
	Q_OBJECT

public:
	explicit TownEventsWidget(CGTownInstance &, QWidget * parent = nullptr);
	~TownEventsWidget();

	void obtainData();
	void commitChanges(MapController & controller);
private slots:
	void on_timedEventAdd_clicked();
	void on_timedEventRemove_clicked();
	void on_eventsList_itemActivated(QListWidgetItem * item);

private:

	Ui::TownEventsWidget * ui;
	CGTownInstance & town;
};

class TownEventsDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;

	TownEventsDelegate(CGTownInstance &, MapController &);

	QWidget* createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
	void setEditorData(QWidget * editor, const QModelIndex & index) const override;
	void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;
	void updateModelData(QAbstractItemModel * model, const QModelIndex & index) const override;

private:
	CGTownInstance & town;
	MapController & controller;
};
