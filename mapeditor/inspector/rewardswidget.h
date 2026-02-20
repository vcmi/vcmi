/*
 * rewardswidget.h, part of VCMI engine
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
#include "../lib/mapObjects/CRewardableObject.h"

namespace Ui {
class RewardsWidget;
}

class RewardsWidget : public QDialog
{
	Q_OBJECT

public:
	
	explicit RewardsWidget(CMap &, CRewardableObject &, QWidget *parent = nullptr);
	~RewardsWidget();
	
	void obtainData();
	bool commitChanges();

private slots:
	void on_addVisitInfo_clicked();

	void on_removeVisitInfo_clicked();

	void on_selectMode_currentIndexChanged(int index);

	void on_resetPeriod_valueChanged(int arg1);

	void on_visitInfoList_itemSelectionChanged();

	void on_visitInfoList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

	void on_rCreatureAdd_clicked();

	void on_rCreatureRemove_clicked();

	void on_lCreatureAdd_clicked();

	void on_lCreatureRemove_clicked();

	void on_castSpellCheck_toggled(bool checked);

	void on_bonusAdd_clicked();

	void on_bonusRemove_clicked();

private:
	
	void saveCurrentVisitInfo(int index);
	void loadCurrentVisitInfo(int index);
	
	void onCreatureAdd(QTableWidget * listWidget, QComboBox * comboWidget, QSpinBox * spinWidget);
	
	Ui::RewardsWidget *ui;
	CRewardableObject & object;
	CMap & map;
};

class RewardsDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	RewardsDelegate(CMap &, CRewardableObject &);

	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;
	
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

	QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void updateModelData(QAbstractItemModel * model, const QModelIndex & index) const override;

private:
	CRewardableObject & object;
	CMap & map;
};
