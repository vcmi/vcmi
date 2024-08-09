/*
 * towneventdialog.h, part of VCMI engine
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
	class TownEventDialog;
}

class TownEventDialog : public QDialog
{
	Q_OBJECT

public:
	explicit TownEventDialog(CGTownInstance & town, QListWidgetItem * item, QWidget * parent);
	~TownEventDialog();


private slots:
	void onItemChanged(const QStandardItem * item);
	void on_TownEventDialog_finished(int result);
	void on_okButton_clicked();
	void setRowColumnCheckState(const QStandardItem * item, int column, Qt::CheckState checkState);

private:
	void initPlayers();
	void initResources();
	void initBuildings();
	void initCreatures();

	QVariant playersToVariant();
	QVariantMap resourcesToVariant();
	QVariantList buildingsToVariant();
	QVariantList creaturesToVariant();

	QStandardItem * addBuilding(const CTown & ctown, BuildingID bId, std::set<si32> & remaining);

	Ui::TownEventDialog * ui;
	CGTownInstance & town;
	QListWidgetItem * townEventListItem;
	QMap<QString, QVariant> params;
	QStandardItemModel buildingsModel;
};
