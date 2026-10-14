/*
 * questwidget.h, part of VCMI engine
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
#include "../../lib/mapObjects/Quest.h"


namespace Ui {
class QuestWidget;
}

class MapController;

class QuestWidget : public QDialog
{
	Q_OBJECT

public:
	explicit QuestWidget(MapController &, Quest &, QWidget *parent = nullptr);
	~QuestWidget();
	
	void obtainData();
	bool commitChanges();

private slots:
	void onTargetPicked(const CGObjectInstance *);
	
	void on_lKillTargetSelect_clicked();

	void on_lCreatureAdd_clicked();

	void on_lCreatureRemove_clicked();

private:
	void onCreatureAdd(QTableWidget * listWidget, QComboBox * comboWidget, QSpinBox * spinWidget);
	
	Quest & quest;
	MapController & controller;
	Ui::QuestWidget *ui;
};

class QuestDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;
	
	QuestDelegate(MapController &, Quest &);
	
	QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
	void setEditorData(QWidget * editor, const QModelIndex & index) const override;
	void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;
	void updateModelData(QAbstractItemModel * model, const QModelIndex & index) const override;
	
protected:
	bool eventFilter(QObject * object, QEvent * event) override;

private:
	Quest & quest;
	MapController & controller;
};
