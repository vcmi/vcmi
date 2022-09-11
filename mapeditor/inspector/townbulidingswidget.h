#ifndef TOWNBULIDINGSWIDGET_H
#define TOWNBULIDINGSWIDGET_H

#include <QDialog>
#include "../lib/mapObjects/CGTownInstance.h"

namespace Ui {
class TownBulidingsWidget;
}

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

#endif // TOWNBULIDINGSWIDGET_H
