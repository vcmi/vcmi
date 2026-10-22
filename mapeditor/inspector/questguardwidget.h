/*
 * QuestGuardWidget.h, part of VCMI engine
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
class QuestGuardWidget;
}

class MapController;

class QuestGuardWidget : public QDialog
{
	Q_OBJECT

public:
	explicit QuestGuardWidget(MapController &, Quest &, QWidget *parent = nullptr);
	~QuestGuardWidget();
	
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
	Ui::QuestGuardWidget *ui;
};

class QuestGuardDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;
	
	QuestGuardDelegate(MapController &, Quest &);
	
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
