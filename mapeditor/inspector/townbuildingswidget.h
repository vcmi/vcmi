/*
 * townbuildingswidget.h, part of VCMI engine
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
#include "../lib/mapObjects/CGTownInstance.h"

namespace Ui {
class TownBuildingsWidget;
}

std::string defaultBuildingIdConversion(BuildingID bId);

QStandardItem * getBuildingParentFromTreeModel(const CBuilding * building, const QStandardItemModel & model);

QVariantList getBuildingVariantsFromModel(const QStandardItemModel & model, int modelColumn, Qt::CheckState checkState);

class TownBuildingsWidget : public QDialog
{
	Q_OBJECT

	QStandardItem * addBuilding(const CTown & ctown, int bId, std::set<si32> & remaining);
	
public:
	enum Column
	{
		TYPE, ENABLED, BUILT
	};
	explicit TownBuildingsWidget(CGTownInstance &, QWidget *parent = nullptr);
	~TownBuildingsWidget();

	void addBuildings(const CTown & ctown);
	std::set<BuildingID> getForbiddenBuildings();
	std::set<BuildingID> getBuiltBuildings();

private slots:
	void on_treeView_expanded(const QModelIndex &index);

	void on_treeView_collapsed(const QModelIndex &index);

	void on_buildAll_clicked();

	void on_demolishAll_clicked();

	void on_enableAll_clicked();

	void on_disableAll_clicked();

	void onItemChanged(const QStandardItem * item);

private:
	std::set<BuildingID> getBuildingsFromModel(int modelColumn, Qt::CheckState checkState);
	void setRowColumnCheckState(const QStandardItem * item, Column column, Qt::CheckState checkState);
	void setAllRowsColumnCheckState(Column column, Qt::CheckState checkState);

	Ui::TownBuildingsWidget *ui;
	CGTownInstance & town;
	mutable QStandardItemModel model;
};

class TownBuildingsDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	using QStyledItemDelegate::QStyledItemDelegate;
	
	TownBuildingsDelegate(CGTownInstance &);
	
	QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	
private:
	CGTownInstance & town;
	//std::set<BuildingID>
};

