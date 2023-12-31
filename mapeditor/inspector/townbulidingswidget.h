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
class TownBulidingsWidget;
}

std::string defaultBuildingIdConversion(BuildingID bId);

class TownBulidingsWidget : public QDialog
{
	Q_OBJECT

	QStandardItem * addBuilding(const CTown & ctown, int bId, std::set<si32> & remaining);
	
public:
	explicit TownBulidingsWidget(CGTownInstance &, QWidget *parent = nullptr);
	~TownBulidingsWidget();
	
	void addBuildings(const CTown & ctown);
	std::set<BuildingID> getForbiddenBuildings();
	std::set<BuildingID> getBuiltBuildings();

private slots:
	void on_treeView_expanded(const QModelIndex &index);

	void on_treeView_collapsed(const QModelIndex &index);

private:
	std::set<BuildingID> getBuildingsFromModel(int modelColumn, Qt::CheckState checkState);
	
	Ui::TownBulidingsWidget *ui;
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

